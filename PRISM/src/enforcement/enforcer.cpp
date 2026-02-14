#include "prism.hpp"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using prism::utils::Logger;

namespace prism::enforcement {

    using prism::RiskDecision;
    using prism::RiskLevel;
    using prism::utils::Logger;

    class EnforcementEngine {
        public:
            std::string apply(const RiskDecision& decision) {
                switch (decision.level) {
                    case RiskLevel::Low:
                        return "Monitoring only";
                    case RiskLevel::Medium:
                        return "Alert operators";
                    case RiskLevel::High:
                        if (decision.restrictWrites) {
                            if (!locked_) {
                                if (sendSafetyCoil(0xFF00)) {
                                    locked_ = true;
                                    Logger::instance().error(
                                        "ENFORCEMENT: Safety coil 0xFFFF engaged — "
                                        "write-lock signal sent to PLC (Modbus FC5)");
                                    return "Write-lock ENGAGED (coil 0xFFFF)";
                                } else {
                                    return "Write-lock FAILED (could not reach PLC)";
                                }
                            }
                            return "Write-lock active (already engaged)";
                        } else {
                            return "Escalation without restriction";
                        }
                }
                return "";
            }

        private:
            bool locked_{false};

            bool sendSafetyCoil(uint16_t value) {
                int fd = socket(AF_INET, SOCK_STREAM, 0);
                if (fd < 0) return false;

                sockaddr_in sa{};
                sa.sin_family = AF_INET;
                sa.sin_port = htons(502);
                inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

                if (connect(fd, (sockaddr*)&sa, sizeof(sa)) < 0) {
                    close(fd);
                    return false;
                }

                // Modbus FC5 Write Single Coil — address 0xFFFF
                uint8_t pdu[12] = {
                    0x00, 0x01,                     // TX id
                    0x00, 0x00,                     // protocol id
                    0x00, 0x06,                     // length
                    0x01,                           // unit id
                    0x05,                           // FC5 Write Single Coil
                    0xFF, 0xFF,                     // coil address 0xFFFF
                    (uint8_t)(value >> 8),           // value high
                    (uint8_t)(value & 0xFF)          // value low
                };

                ssize_t sent = write(fd, pdu, sizeof(pdu));
                close(fd);
                return sent == sizeof(pdu);
            }
    };

}
