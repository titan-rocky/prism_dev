#include "prism.hpp"

namespace prism::parser {

    Proto classify_protocol(const RawPacket& p) {

        if (p.srcPort == 502 || p.dstPort == 502) return Proto::MODBUS;

        if (p.srcPort == 20000 || p.dstPort == 20000) return Proto::DNP3;

        if (p.srcPort == 2404 || p.dstPort == 2404) return Proto::IEC104;

        return Proto::UNKNOWN;
    }

}

