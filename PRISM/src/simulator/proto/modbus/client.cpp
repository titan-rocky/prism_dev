#include <cstdio>
#include <cstdint>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

namespace proto::modbus {

    struct PDU {
        uint16_t transaction{1};
        uint16_t protocol{0};
        uint16_t length{6};
        uint8_t  unit{1};
        uint8_t  fn{0};
        uint16_t addr{0};
        uint16_t value{0};
    };

    static std::vector<uint8_t> encode_mbap_pdu(const PDU& p) {
        std::vector<uint8_t> b(12);
        b[0] = p.transaction >> 8; b[1] = p.transaction & 0xFF;
        b[2] = p.protocol >> 8;    b[3] = p.protocol & 0xFF;
        b[4] = p.length >> 8;      b[5] = p.length & 0xFF;
        b[6] = p.unit;
        b[7] = p.fn;
        b[8] = p.addr >> 8;        b[9] = p.addr & 0xFF;
        b[10] = p.value >> 8;      b[11] = p.value & 0xFF;
        return b;
    }

    int run_client(int port, uint8_t fn, uint16_t addr, uint16_t val) {
        static int fd = -1;

        if (fd < 0) {
            fd = socket(AF_INET, SOCK_STREAM, 0);
            if (fd < 0) { std::perror("socket"); return 1; }

            sockaddr_in sa{};
            sa.sin_family = AF_INET;
            sa.sin_port   = htons(port);
            inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

            if (connect(fd, (sockaddr*)&sa, sizeof(sa)) < 0) {
                std::perror("connect");
                close(fd);
                fd = -1;
                return 1;
            }
        }

        PDU p{};
        p.fn = fn;
        p.addr = addr;
        p.value = val;

        auto buf = encode_mbap_pdu(p);

        if (send(fd, buf.data(), buf.size(), 0) < 0) {
            std::perror("send");
            close(fd);
            fd = -1;
            return 1;
        }

        uint8_t rx[256];
        recv(fd, rx, sizeof(rx), 0);

        return 0;
    }

}

