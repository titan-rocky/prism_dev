#include "prism.hpp"
#include <unordered_map>

namespace prism::parser {

    struct FlowKey {
        std::string src;
        std::string dst;
        uint16_t sport{};
        uint16_t dport{};

        bool operator==(const FlowKey& o) const {
            return src==o.src && dst==o.dst &&
                sport==o.sport && dport==o.dport;
        }
    };

    struct FlowKeyHash {
        size_t operator()(const FlowKey& k) const {
            return std::hash<std::string>()(k.src)
                ^ std::hash<std::string>()(k.dst)
                ^ (k.sport<<1) ^ (k.dport<<2);
        }
    };

    struct FlowState {
        bool client_is_src{};
        prism::Timestamp last_seen;
    };

    class FlowTable {
        public:
            FlowState classify(const RawPacket& pkt) {
                FlowKey k{pkt.srcIp, pkt.dstIp, pkt.srcPort, pkt.dstPort};
                FlowKey r{pkt.dstIp, pkt.srcIp, pkt.dstPort, pkt.srcPort};

                auto now = pkt.timestamp;

                // Known forward flow
                if (auto it = table_.find(k); it != table_.end()) {
                    it->second.last_seen = now;
                    return it->second;
                }

                // Known reverse flow → mark as response
                if (auto it = table_.find(r); it != table_.end()) {
                    it->second.last_seen = now;
                    FlowState fs = it->second;
                    fs.client_is_src = !fs.client_is_src;
                    return fs;
                }

                // New flow — assume initiator is client
                FlowState fs{};
                fs.client_is_src = true;
                fs.last_seen = now;
                table_[k] = fs;
                return fs;
            }

        private:
            std::unordered_map<FlowKey, FlowState, FlowKeyHash> table_;
    };

}

