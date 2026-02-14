#include "common.hpp"
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <atomic>

namespace proto::modbus {

    static std::atomic<bool> write_locked{false};

    static bool is_write_fc(uint8_t fc) {
        return fc == 5 || fc == 6 || fc == 15 || fc == 16;
    }

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

                // Safety coil 0xFFFF — write-lock control
                if (p.address == 0xFFFF && p.function == 5) {
                    if (p.value == 0xFF00) {
                        write_locked.store(true);
                        std::printf("[modbus] SAFETY COIL ENGAGED — writes locked\n");
                    } else if (p.value == 0x0000) {
                        write_locked.store(false);
                        std::printf("[modbus] SAFETY COIL RELEASED — writes allowed\n");
                    }
                    auto resp = encode_mbap_pdu(p);
                    write(cfd, resp.data(), resp.size());
                }
                // Write-locked: reject write-class FCs with exception
                else if (write_locked.load() && is_write_fc(p.function)) {
                    std::printf("[modbus] BLOCKED fn=%u (write-locked)\n", p.function);
                    auto exc = encode_exception(p.function, 0x01);
                    write(cfd, exc.data(), exc.size());
                }
                // Normal operation
                else {
                    auto resp = encode_mbap_pdu(p);
                    write(cfd, resp.data(), resp.size());
                }
            }

            close(cfd);
        }
    }

}


