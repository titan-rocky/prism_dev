#pragma once

#include <cstdint>
#include <vector>

namespace proto::modbus {

    struct PDU {
        uint8_t  function{0};
        uint16_t address{0};
        uint16_t value{0};
    };

    inline std::vector<uint8_t> encode_mbap_pdu(const PDU& p)
    {
        std::vector<uint8_t> buf;

        buf.resize(12);

        buf[0] = 0x00; buf[1] = 0x01;   // TX id
        buf[2] = 0x00; buf[3] = 0x00;   // protocol id
        buf[4] = 0x00; buf[5] = 0x06;   // length
        buf[6] = 0x01;                  // unit id

        buf[7]  = p.function;
        buf[8]  = (p.address >> 8) & 0xFF;
        buf[9]  = (p.address     ) & 0xFF;
        buf[10] = (p.value   >> 8) & 0xFF;
        buf[11] = (p.value       ) & 0xFF;

        return buf;
    }

    // Modbus Exception Response: fc | 0x80, followed by exception code
    // Code 0x01 = Illegal Function
    inline std::vector<uint8_t> encode_exception(uint8_t fc, uint8_t exception_code)
    {
        std::vector<uint8_t> buf(9);

        buf[0] = 0x00; buf[1] = 0x01;   // TX id
        buf[2] = 0x00; buf[3] = 0x00;   // protocol id
        buf[4] = 0x00; buf[5] = 0x03;   // length (unit + fc + code)
        buf[6] = 0x01;                  // unit id
        buf[7] = fc | 0x80;             // error function code
        buf[8] = exception_code;

        return buf;
    }

    inline PDU decode_mbap_pdu(const std::vector<uint8_t>& b)
    {
        PDU p{};
        if (b.size() < 12) return p;

        p.function = b[7];
        p.address  = (b[8] << 8) | b[9];
        p.value    = (b[10] << 8) | b[11];

        return p;
    }

}

