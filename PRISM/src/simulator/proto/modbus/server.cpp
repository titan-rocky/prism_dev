#include "common.hpp"
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace proto::modbus {

    int start_server(int port)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            std::perror("socket");
            return 1;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::perror("bind");
            close(fd);
            return 1;
        }

        listen(fd, 1);
        std::printf("[modbus] listening on %d\n", port);

        while (true)
        {
            int cfd = accept(fd, nullptr, nullptr);
            if (cfd < 0) continue;

            uint8_t buf[256];
            ssize_t n = read(cfd, buf, sizeof(buf));
            if (n > 0)
            {
                std::vector<uint8_t> v(buf, buf+n);
                PDU p = decode_mbap_pdu(v);

                std::printf("[modbus] RX fn=%u addr=%u val=%u\n",
                        p.function, p.address, p.value);

                auto resp = encode_mbap_pdu(p);
                write(cfd, resp.data(), resp.size());
            }

            close(cfd);
        }
    }

}

