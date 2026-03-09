#include "prism.hpp"
#include "flow_table.cpp"

namespace prism::parser {

    static FlowTable flowTable;

    class ModbusParser {
        public:
            ParsedRecord parse(const RawPacket& packet) {

                ParsedRecord rec;
                rec.timestamp = packet.timestamp;
                rec.protocol  = "Modbus";

                const auto& b = packet.payload;
                const size_t n = b.size();

                if (n < 12) {
                    rec.protocol = "Unknown";
                    return rec;
                }

                // Explicitly mark request direction from client->server
                rec.isRequest = (packet.dstPort == 502);

                // Fixed MBAP → PDU offset
                const size_t pdu = 7;

                rec.functionCode = b[pdu];
                rec.address      = (b[pdu+1] << 8) | b[pdu+2];
                // Check if function code is a Write operation (5, 6, 15, 16)
                rec.isWrite = (rec.functionCode == 5 || rec.functionCode == 6 || 
                               rec.functionCode == 15 || rec.functionCode == 16);

                return rec;

            }
    };

}

