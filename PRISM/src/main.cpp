#include "prism.hpp"

#include "capture/pcap.cpp"
#include "parser/modbus_parser.cpp"
#include "parser/dnp3_parser.cpp"
#include "parser/iec104_parser.cpp"
#include "parser/proto_classifier.cpp"
#include "window/window_manager.cpp"
#include "feature/feat_miner.cpp"
#include "detection/hst.cpp"
#include "detection/burst.cpp"
#include "risk/risk_model.cpp"
#include "enforcement/enforcer.cpp"
#include "alert/alert_system.cpp"
#include "utils/viz_utils.cpp"

#include <iomanip>

#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <atomic>
#include <thread>

using prism::utils::Logger;

int main(int argc, char** argv) {
    using namespace prism;

    if (argc < 2) {
        std::cerr << "Usage: prism <interface>\n";
        return 1;
    }

    const std::string iface = argv[1];

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    if (pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0) {
        std::perror("pthread_sigmask");
        return 1;
    }

    Logger::instance().info("PRISM starting on interface " + iface);

    std::atomic<bool> running(true);

    capture::PcapCapture capture(iface, true);
    parser::ModbusParser modbusParser;
    parser::DNP3Parser dnp3Parser;
    parser::IEC104Parser iec104Parser;

    windowing::WindowManager windowManager(std::chrono::milliseconds(5000), std::chrono::milliseconds(1000));
    features::FeatureExtractor extractor;
    detection::HalfSpaceTrees hst;
    detection::BurstDetector burst;
    risk::RiskModel riskModel;
    enforcement::EnforcementEngine enforcer;
    alert::AlertDispatcher dispatcher;

    auto fcName = [](const std::string& proto, uint8_t fc) -> std::string {
        if (proto == "Modbus") {
            switch (fc) {
                case 1: return "Read Coils";
                case 2: return "Read Discrete Inputs";
                case 3: return "Read Holding Registers";
                case 4: return "Read Input Registers";
                case 5: return "Write Single Coil";
                case 6: return "Write Single Register";
                case 8: return "Diagnostics";
                case 15: return "Write Multiple Coils";
                case 16: return "Write Multiple Registers";
                default: return "Unknown";
            }
        }
        if (proto == "DNP3") {
            switch (fc) {
                case 1: return "Read";
                case 2: return "Write";
                case 3: return "Select";
                case 4: return "Operate";
                case 130: return "Unsolicited Response";
                default: return "FC " + std::to_string(fc);
            }
        }
        if (proto == "IEC104") {
            switch (fc) {
                case 1: return "Single Point Info";
                case 13: return "Measured Float";
                case 30: return "Single Point w/ Timestamp";
                case 45: return "Single Command";
                case 46: return "Double Command";
                case 100: return "Interrogation";
                default: return "Type " + std::to_string(fc);
            }
        }
        return "Unknown";
    };

    RawPacket packet;
    ParsedRecord record;
    Window window;

    std::thread sig_watcher([&capture, &running]() {
        sigset_t wait_mask;
        sigemptyset(&wait_mask);
        sigaddset(&wait_mask, SIGINT);
        sigaddset(&wait_mask, SIGTERM);

        int sig = 0;
        int rc = sigwait(&wait_mask, &sig);
        if (rc == 0) {
            capture.stop();
            running.store(false);
            prism::utils::Logger::instance().info(
                std::string("Signal received (") + std::to_string(sig) + "), shutting down."
            );
        } else {
            prism::utils::Logger::instance().warn("sigwait failed");
        }
    });
    sig_watcher.detach();

    int pcapFd = capture.getSelectableFd();

    if (pcapFd >= 0) {
        struct pollfd fds[1];
        fds[0].fd = pcapFd;
        fds[0].events = POLLIN;

        const int poll_timeout_ms = 1000;

        while (running.load()) {
            int rc = poll(fds, 1, poll_timeout_ms);
            if (rc < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (rc == 0) continue;

            if (fds[0].revents & POLLIN) {
                int drained = 0;
                const int max_drain = 1024;

                while (running.load() && drained < max_drain && capture.nextPacket(packet)) {

                    parser::Proto proto = parser::classify_protocol(packet);
                    if (proto == parser::Proto::UNKNOWN) {
                        ++drained;
                        continue;
                    }

                    record = ParsedRecord();

                    if (proto == parser::Proto::MODBUS) record = modbusParser.parse(packet);
                    else if (proto == parser::Proto::DNP3) record = dnp3Parser.parse(packet);
                    else if (proto == parser::Proto::IEC104) record = iec104Parser.parse(packet);
                    else continue;

                    if (record.protocol.empty() || record.protocol == "Unknown") continue;

                    if (!windowManager.addRecord(record, window)) {
                        ++drained;
                        continue;
                    }

                    if (window.records.empty()) continue; // Skip empty windows

                    FeatureVector fv = extractor.compute(window);
                    double hstScore = hst.score(fv);
                    double burstScore = burst.score(fv);

                    DetectionResult det = riskModel.fuse(hstScore, burstScore);
                    RiskDecision decision = riskModel.evaluate(det);

                    std::string action = enforcer.apply(decision);

                    // --- Alert line ---
                    std::string label = utils::getFuzzyLabel(decision.probability);
                    std::string bar = utils::getProgressBar(decision.probability);
                    std::ostringstream pct;
                    pct << std::fixed << std::setprecision(2) << (decision.probability * 100.0) << "%";

                    if (decision.level == RiskLevel::High) {
                        Logger::instance().error(
                            "ALERT: " + label + " " + bar + " Risk: " + pct.str() + " — " + action);
                    } else if (decision.level == RiskLevel::Medium) {
                        Logger::instance().warn(
                            "ALERT: " + label + " " + bar + " Risk: " + pct.str() + " — " + action);
                    } else {
                        Logger::instance().info(
                            "ALERT: " + label + " " + bar + " Risk: " + pct.str() + " — " + action);
                    }

                    // --- Packet identity ---
                    std::string dir = record.isRequest ? "Request" : "Response";
                    std::string fcLabel = fcName(record.protocol, record.functionCode);
                    Logger::instance().info(
                        " " + record.protocol + " " + dir +
                        " | FC " + std::to_string(record.functionCode) +
                        " (" + fcLabel + ")" +
                        " | Address: " + std::to_string(record.address));

                    // --- Analysis metrics ---
                    std::ostringstream mv;
                    mv << std::fixed << std::setprecision(0);
                    mv << " Packets: " << fv.values[0]
                       << " | Write%: " << (fv.values[1] * 100.0);
                    mv << std::fixed << std::setprecision(2);
                    mv << " | Func Spread: " << fv.values[2]
                       << " | Entropy: " << fv.values[3]
                       << " | Proto Mix: " << fv.values[4]
                       << " | Density: " << fv.values[7];
                    Logger::instance().info(mv.str());

                    ++drained;
                }
            }
        }
    } else {
        Logger::instance().warn("pcap_get_selectable_fd returned -1; using timeout fallback.");

        while (running.load()) {
            if (!capture.nextPacket(packet)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            parser::Proto proto = parser::classify_protocol(packet);
            if (proto == parser::Proto::UNKNOWN)
                continue;

            record = ParsedRecord();

            if (proto == parser::Proto::MODBUS) record = modbusParser.parse(packet);
            else if (proto == parser::Proto::DNP3) record = dnp3Parser.parse(packet);
            else if (proto == parser::Proto::IEC104) record = iec104Parser.parse(packet);
            else continue;

            if (record.protocol.empty() || record.protocol == "Unknown") continue;

            if (!windowManager.addRecord(record, window)) continue;

            if (window.records.empty()) continue;

            FeatureVector fv = extractor.compute(window);
            double hstScore = hst.score(fv);
            double burstScore = burst.score(fv);

            DetectionResult det = riskModel.fuse(hstScore, burstScore);
            RiskDecision decision = riskModel.evaluate(det);

            std::string action = enforcer.apply(decision);

            std::string label = utils::getFuzzyLabel(decision.probability);
            std::string bar = utils::getProgressBar(decision.probability);
            std::ostringstream pct;
            pct << std::fixed << std::setprecision(2) << (decision.probability * 100.0) << "%";

            if (decision.level == RiskLevel::High) {
                Logger::instance().error(
                    "ALERT: " + label + " " + bar + " Risk: " + pct.str() + " — " + action);
            } else if (decision.level == RiskLevel::Medium) {
                Logger::instance().warn(
                    "ALERT: " + label + " " + bar + " Risk: " + pct.str() + " — " + action);
            } else {
                Logger::instance().info(
                    "ALERT: " + label + " " + bar + " Risk: " + pct.str() + " — " + action);
            }

            std::string dir = record.isRequest ? "Request" : "Response";
            std::string fcLabel = fcName(record.protocol, record.functionCode);
            Logger::instance().info(
                " " + record.protocol + " " + dir +
                " | FC " + std::to_string(record.functionCode) +
                " (" + fcLabel + ")" +
                " | Address: " + std::to_string(record.address));

            std::ostringstream mv;
            mv << std::fixed << std::setprecision(0);
            mv << " Packets: " << fv.values[0]
               << " | Write%: " << (fv.values[1] * 100.0);
            mv << std::fixed << std::setprecision(2);
            mv << " | Func Spread: " << fv.values[2]
               << " | Entropy: " << fv.values[3]
               << " | Proto Mix: " << fv.values[4]
               << " | Density: " << fv.values[7];
            Logger::instance().info(mv.str());
        }
    }

    capture.stop();
    Logger::instance().info("PRISM shutting down.");
    return 0;
}

