#include <cstdio>
#include <cstdint>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

namespace proto {
    namespace dnp3 {

        static int make_server_socket(int port) {
            int s = socket(AF_INET, SOCK_STREAM, 0);
            if (s < 0) return -1;

            int yes = 1;
            setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;

            if (bind(s, (sockaddr*)&addr, sizeof(addr)) < 0) {
                close(s);
                return -1;
            }

            if (listen(s, 4) < 0) {
                close(s);
                return -1;
            }

            return s;
        }

        static void decode_and_log(const std::vector<uint8_t>& buf) {
            if (buf.size() < 10) {
                printf("[dnp3] too small frame (%zu)\n", buf.size());
                return;
            }

            uint8_t ctrl = buf[2];
            bool is_from_master = (ctrl & 0x40);

            // uint8_t func = buf[10];
            uint8_t func = buf[6];

            printf("[dnp3] RX master=%d fc=%u\n",
                    is_from_master ? 1 : 0,
                    func);
        }

        int start_server(int port) {
            int srv = make_server_socket(port);
            if (srv < 0) {
                printf("[dnp3] failed to bind %d\n", port);
                return 1;
            }

            printf("[dnp3] listening on %d\n", port);

            while (true) {
                int c = accept(srv, nullptr, nullptr);
                if (c < 0) continue;

                std::vector<uint8_t> buf(2048);
                ssize_t n = read(c, buf.data(), buf.size());

                if (n > 0) {
                    buf.resize(n);
                    decode_and_log(buf);

                    // echo back same frame
                    write(c, buf.data(), buf.size());
                }

                close(c);
            }

            close(srv);
            return 0;
        }

    } }

