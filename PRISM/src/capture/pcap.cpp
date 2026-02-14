#include "prism.hpp"

#include <pcap.h>
#include <pcap/pcap.h>
#include <stdexcept>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <mutex>
#include <atomic>

namespace prism::capture {

    class PcapCapture {
        public:
            PcapCapture(const std::string& deviceName, bool promiscuous)
            {
                char errbuf[PCAP_ERRBUF_SIZE]{};
                const int timeoutMs = 1000;

                pcap_t* h = pcap_open_live(
                    deviceName.c_str(),
                    65535,
                    promiscuous ? 1 : 0,
                    timeoutMs,
                    errbuf
                );

                if (!h) {
                    throw std::runtime_error(std::string("pcap_open_live failed: ") + errbuf);
                }

                linktype_ = pcap_datalink(h);

                const char* filterExpr = "ip";
                bpf_program fp{};
                if (pcap_compile(h, &fp, filterExpr, 1, PCAP_NETMASK_UNKNOWN) == 0) {
                    pcap_setfilter(h, &fp);
                    pcap_freecode(&fp);
                }

                {
                    std::lock_guard<std::mutex> lg(handle_mtx_);
                    handle_ = h;
                    active_.store(true, std::memory_order_release);
                }
            }

            ~PcapCapture() {
                pcap_t* local = nullptr;
                {
                    std::lock_guard<std::mutex> lg(handle_mtx_);
                    local = handle_;
                    handle_ = nullptr;
                    active_.store(false, std::memory_order_release);
                }
                if (local) pcap_close(local);
            }

            void stop() {
                pcap_t* local = nullptr;
                {
                    std::lock_guard<std::mutex> lg(handle_mtx_);
                    if (!handle_ || !active_.load(std::memory_order_acquire)) return;
                    local = handle_;
                    active_.store(false, std::memory_order_release);
                }
                if (local) pcap_breakloop(local);
            }

            bool nextPacket(RawPacket& packet) {
                pcap_pkthdr* header = nullptr;
                const u_char* data = nullptr;
                pcap_t* local = nullptr;

                {
                    std::lock_guard<std::mutex> lg(handle_mtx_);
                    if (!handle_ || !active_.load(std::memory_order_acquire)) return false;
                    local = handle_;
                }

                int res = pcap_next_ex(local, &header, &data);
                if (res == 0) return false;
                if (!(res == 1 && header && data)) return false;

                const int l2 =
                    (linktype_ == DLT_NULL)   ? 4  :
                    (linktype_ == DLT_EN10MB) ? 14 :
                                                0;

                if (header->len < l2 + sizeof(ip)) return false;

                const ip* ipHdr = reinterpret_cast<const ip*>(data + l2);
                if (ipHdr->ip_v != 4) return false;

                const uint8_t ihl = ipHdr->ip_hl * 4;
                if (ihl < 20 || header->len < l2 + ihl) return false;

                packet.timestamp = std::chrono::steady_clock::now();

                char srcBuf[INET_ADDRSTRLEN]{};
                char dstBuf[INET_ADDRSTRLEN]{};
                inet_ntop(AF_INET, &ipHdr->ip_src, srcBuf, sizeof(srcBuf));
                inet_ntop(AF_INET, &ipHdr->ip_dst, dstBuf, sizeof(dstBuf));
                packet.srcIp = srcBuf;
                packet.dstIp = dstBuf;

                const uint8_t proto = ipHdr->ip_p;
                const uint8_t* l4 = data + l2 + ihl;

                packet.srcPort = 0;
                packet.dstPort = 0;

                if (proto == IPPROTO_TCP && header->len >= l2 + ihl + 20) {
                    const uint8_t tcp_hl = ((l4[12] & 0xF0) >> 4) * 4;
                    if (tcp_hl < 20 || header->len < l2 + ihl + tcp_hl) return false;

                    packet.srcPort = (l4[0] << 8) | l4[1];
                    packet.dstPort = (l4[2] << 8) | l4[3];

                    const uint8_t* pdu = l4 + tcp_hl;
                    packet.payload.assign(pdu, data + header->len);
                } else {
                    packet.payload.assign(data + l2 + ihl, data + header->len);
                }

                return true;
            }

            int getSelectableFd() const {
                std::lock_guard<std::mutex> lg(handle_mtx_);
                if (!handle_) return -1;
                return pcap_get_selectable_fd(handle_);
            }

        private:
            mutable std::mutex handle_mtx_;
            pcap_t* handle_{nullptr};
            std::atomic<bool> active_{false};
            int linktype_{DLT_EN10MB};
    };

}

