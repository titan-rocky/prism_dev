#include <cstdio>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

namespace proto {
    namespace iec104 {

        static int make_listen_socket(int port)
        {
            int s = socket(AF_INET, SOCK_STREAM, 0);
            if (s < 0) return -1;

            int opt = 1;
            setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;

            if (bind(s, (sockaddr*)&addr, sizeof(addr)) < 0)
                return -1;

            if (listen(s, 8) < 0)
                return -1;

            return s;
        }

        int start_server(int port)
        {
            int ls = make_listen_socket(port);
            if (ls < 0) {
                printf("[iec104] listen failed\n");
                return 1;
            }

            printf("[iec104] server listening %d\n", port);

            while (true)
            {
                int cs = accept(ls, nullptr, nullptr);
                if (cs < 0) continue;

                std::vector<uint8_t> buf(256);
                ssize_t n = read(cs, buf.data(), buf.size());

                // printf("[iec104] DBG len=%zu\n", n);
                // for (size_t i = 0; i < n; ++i)
                //     printf(" %02X", buf[i]);
                // printf("\n");


                if (n >= 7) {
                    uint8_t type_id = buf[6];
                    printf("[iec104] RX type=%u\n", type_id);
                }

                write(cs, buf.data(), n); // echo
                close(cs);
            }
        }

    } }

