#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>

namespace proto::modbus {
    int start_server(int port);
    int run_client(int port, uint8_t fn, uint16_t addr, uint16_t val);
}

namespace proto::dnp3 {
    int start_server(int port);
    int run_client(int port, uint8_t fn);
}

namespace proto::iec104 {
    int start_server(int port);
    int run_client(int port, uint8_t fn);
}

namespace scada_sim {

    enum class Mode { START_SERVER, REQUEST, INVALID };
    enum class Protocol { MODBUS, DNP3, IEC104, UNKNOWN };

    static Mode parse_mode(const std::string& s) {
        if (s == "start-server") return Mode::START_SERVER;
        if (s == "request")      return Mode::REQUEST;
        return Mode::INVALID;
    }

    static Protocol parse_protocol(const std::string& s) {
        if (s == "modbus") return Protocol::MODBUS;
        if (s == "dnp3")   return Protocol::DNP3;
        if (s == "iec104") return Protocol::IEC104;
        return Protocol::UNKNOWN;
    }

    static int usage() {
        std::fprintf(stderr,
                "scada-sim start-server <protocol> <port>\n"
                "scada-sim request <protocol> <port> <fn> [addr val]\n"
                );
        return 1;
    }

    int run_cli(int argc, char** argv) {

        if (argc < 4)
            return usage();

        Mode mode = parse_mode(argv[1]);
        Protocol proto = parse_protocol(argv[2]);
        int port = std::stoi(argv[3]);

        if (mode == Mode::INVALID || proto == Protocol::UNKNOWN)
            return usage();

        if (mode == Mode::START_SERVER) {
            if (proto == Protocol::MODBUS)
                return proto::modbus::start_server(port);
            if (proto == Protocol::DNP3)
                return proto::dnp3::start_server(port);
            if (proto == Protocol::IEC104)
                return proto::iec104::start_server(port);
            return usage();
        }

        if (mode == Mode::REQUEST) {

            if (argc < 5)
                return usage();

            uint8_t fn = static_cast<uint8_t>(std::stoi(argv[4]));

            if (proto == Protocol::MODBUS) {

                if (argc < 7)
                    return usage();

                uint16_t addr = static_cast<uint16_t>(std::stoi(argv[5]));
                uint16_t val  = static_cast<uint16_t>(std::stoi(argv[6]));

                return proto::modbus::run_client(port, fn, addr, val);
            }

            if (proto == Protocol::DNP3)
                return proto::dnp3::run_client(port, fn);

            if (proto == Protocol::IEC104) {
                uint8_t type = static_cast<uint8_t>(std::stoi(argv[4]));
                return proto::iec104::run_client(port, type);
            }


            return usage();
        }

        return usage();
    }

}

int main(int argc, char** argv) {
    return scada_sim::run_cli(argc, argv);
}

