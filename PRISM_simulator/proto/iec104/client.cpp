#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

namespace proto {
    namespace iec104 {

        static int make_client_socket(int port) {
            int s = socket(AF_INET, SOCK_STREAM, 0);
            if (s < 0) return -1;

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            const char* host = std::getenv("TARGET_HOST");
            if (!host) host = "127.0.0.1";

            struct addrinfo hints{}, *res = nullptr;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            if (getaddrinfo(host, nullptr, &hints, &res) != 0 || !res) {
                close(s);
                return -1;
            }
            addr.sin_addr = ((sockaddr_in*)res->ai_addr)->sin_addr;
            freeaddrinfo(res);

            if (connect(s, (sockaddr*)&addr, sizeof(addr)) < 0) {
                close(s);
                return -1;
            }

            return s;
        }

        static std::vector<uint8_t> build_frame(uint8_t typeId,
                uint16_t ioa)
        {
            std::vector<uint8_t> f(12, 0);

            f[0] = 0x68;        // start
            f[1] = 0x0A;        // length = 10 bytes after this

            // APCI (4 bytes, ignore seq sync)
            f[2] = 0x00;
            f[3] = 0x00;
            f[4] = 0x00;
            f[5] = 0x00;

            f[6] = typeId;      // PRISM read starts here

            f[7] = 0x01;        // VSQ (1 element)

            f[8] = 0x06;        // COT = activation (request)

            f[9] = 0x01;        // ASDU addr (dummy)

            f[10] = ioa & 0xFF;       // IOA low
            f[11] = (ioa >> 8) & 0xFF; // IOA high

            return f;
        }


        int run_client(int port, uint8_t type_id)
        {
            int s = make_client_socket(port);
            if (s < 0) {
                printf("[iec104] connect failed\n");
                return 1;
            }

            uint16_t obj = 16; // fixed test addr for PRISM

            auto frame = build_frame(type_id, obj);

            write(s, frame.data(), frame.size());

            std::vector<uint8_t> rx(256);
            read(s, rx.data(), rx.size());

            printf("[iec104] sent type=%u addr=%u\n", type_id, obj);

            close(s);
            return 0;
        }

    } }

