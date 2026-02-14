#include "prism.hpp"

namespace prism::parser {

    class IEC104Parser {
        public:
            ParsedRecord parse(const RawPacket& packet)
            {
                ParsedRecord rec;
                rec.timestamp = packet.timestamp;

                if (packet.srcPort != 2404 && packet.dstPort != 2404)
                    return ParsedRecord{};

                const auto& b = packet.payload;
                const size_t n = b.size();

                if (n < 12 || b[0] != 0x68)
                    return ParsedRecord{};   // drop empty / APCI frames

                rec.protocol = "IEC104";

                uint8_t typeId = b[6];
                uint8_t cot    = b[8];

                if (typeId == 0)
                    return ParsedRecord{};   // ignore supervision / test frames

                rec.functionCode = typeId;
                rec.isRequest = (cot == 6);
                rec.isWrite = (typeId >= 45 && typeId <= 50);

                rec.address =
                    (uint32_t)b[9] |
                    ((uint32_t)b[10] << 8) |
                    ((uint32_t)b[11] << 16);

                return rec;
            }

    };

}

