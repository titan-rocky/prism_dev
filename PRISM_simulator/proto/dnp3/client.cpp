#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

namespace proto {
    namespace dnp3 {

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

        static std::vector<uint8_t> build_frame(uint8_t func) {
            std::vector<uint8_t> f(12, 0);

            // link DIR=1 master→outstation
            f[2] = 0x40;

            // application layer begins at byte 6
            f[7] = 0xC0;   // app control: FIR|FIN, master request
            f[6] = func;   // function code (PRISM expects here)

            return f;
        }


        int run_client(int port, uint8_t func) {
            int s = make_client_socket(port);
            if (s < 0) {
                printf("[dnp3] connect failed\n");
                return 1;
            }

            auto frame = build_frame(func);

            write(s, frame.data(), frame.size());

            std::vector<uint8_t> rx(2048);
            read(s, rx.data(), rx.size());

            printf("[dnp3] sent fc=%u\n", func);

            close(s);
            return 0;
        }

    } }

