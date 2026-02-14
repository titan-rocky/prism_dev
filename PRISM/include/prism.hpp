#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace prism {

    using Timestamp = std::chrono::steady_clock::time_point;

    struct RawPacket {
        Timestamp timestamp;
        std::vector<std::uint8_t> payload;
        std::string srcIp;
        std::string dstIp;
        std::uint16_t srcPort{};
        std::uint16_t dstPort{};
    };

    struct ParsedRecord {
        Timestamp timestamp;
        std::string protocol;
        bool isRequest{true};
        bool isWrite{false};
        std::uint8_t functionCode{0};
        std::uint16_t address{0};
    };

    struct Window {
        Timestamp startTime;
        Timestamp endTime;
        std::vector<ParsedRecord> records;
    };

    struct FeatureVector {
        std::vector<double> values;
    };

    struct DetectionResult {
        double hstScore{0.0};
        double burstScore{0.0};
    };

    enum class RiskLevel {
        Low,
        Medium,
        High
    };

    struct RiskDecision {
        RiskLevel level{RiskLevel::Low};
        bool escalate{false};
        bool restrictWrites{false};
        double probability{0.0};
    };

    namespace alert {
        enum class AlertLevel {
            INFO,
            WARNING,
            CRITICAL
        };

        struct AlertContext {
            AlertLevel level;
            double riskProbability;
            std::string message;
        };
    }

    namespace parser {
        enum class Proto {
            UNKNOWN,
            MODBUS,
            DNP3,
            IEC104
        };

        Proto classify_protocol(const RawPacket &p);
    }

    namespace utils {

        class Logger {
            public:
                static Logger& instance() {
                    static Logger logger;
                    return logger;
                }

                void info(const std::string& msg) {
                    log("INFO", msg);
                }

                void warn(const std::string& msg) {
                    log("WARN", msg);
                }

                void error(const std::string& msg) {
                    log("ERROR", msg);
                }

            private:
                Logger() = default;
                Logger(const Logger&) = delete;
                Logger& operator=(const Logger&) = delete;

                std::mutex mutex_;

                void log(const std::string& level, const std::string& msg) {
                    using namespace std::chrono;
                    auto now = std::chrono::system_clock::now();
                    std::time_t t = std::chrono::system_clock::to_time_t(now);
                    std::tm tm{};
#if defined(_WIN32)
                    localtime_s(&tm, &t);
#else
                    localtime_r(&t, &tm);
#endif
                    std::ostringstream os;
                    os << std::put_time(&tm, "%F %T");
                    std::lock_guard<std::mutex> lock(mutex_);
                    std::cerr << "[" << os.str() << "] " << level << ": " << msg << '\n';
                }
        };

    }
}
