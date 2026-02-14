#include "prism.hpp"

namespace prism::parser {

    class DNP3Parser {
        public:
            ParsedRecord parse(const RawPacket& packet)
            {
                ParsedRecord rec;
                rec.timestamp = packet.timestamp;
                rec.protocol = "DNP3";

                if (packet.srcPort != 20000 && packet.dstPort != 20000)
                    return rec;

                const auto& b = packet.payload;
                const size_t n = b.size();

                if (n < 12) {
                    rec.protocol = "Unknown";
                    return rec;
                }

                const uint8_t ctrl = b[2];
                rec.isRequest = ((ctrl & 0x40) != 0);

                rec.functionCode = b[6];

                rec.isWrite =
                    (rec.functionCode == 0x02 ||   // control relay output
                     rec.functionCode == 0x05 ||   // direct operate
                     rec.functionCode == 0x06);    // select

                rec.address = 0;
                return rec;
            }
    };

}

