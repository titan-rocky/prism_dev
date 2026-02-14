#include "prism.hpp"
#include <unordered_map>
#include <cmath>

namespace prism::features {

    class FeatureExtractor {
        public:
        FeatureVector compute(const Window& window) {
            FeatureVector fv;
            fv.values.assign(8, 0.0); // Ensure size 8 for compatibility

            const size_t n = window.records.size();
            if (n == 0) return fv;

            double writeCount = 0.0;
            std::unordered_map<uint8_t, int> funcCounts;
            std::unordered_map<uint16_t, int> addrCounts;
            std::unordered_map<std::string, int> protoCounts;
            
            for (const auto& rec : window.records) {
                if (rec.isWrite) writeCount += 1.0;
                funcCounts[rec.functionCode]++;
                addrCounts[rec.address]++;
                protoCounts[rec.protocol]++;
            }

            double total = static_cast<double>(n);

            double writeRatio = writeCount / total;

            double maxFunc = 0.0;
            for (auto const& [k, v] : funcCounts) {
                if (v > maxFunc) maxFunc = v;
            }
            double funcConc = maxFunc / total;

            double entropy = 0.0;
            for (auto const& [k, v] : addrCounts) {
                double p = static_cast<double>(v) / total;
                if (p > 0) {
                    entropy -= p * std::log2(p);
                }
            }

            double maxProto = 0.0;
            for (auto const& [k, v] : protoCounts) {
                if (v > maxProto) maxProto = v;
            }
            double protoDom = maxProto / total;

            double count = total;

            double meanIA = 0.0;
            double stdIA = 0.0;
            if (n > 1) {
                std::vector<double> deltas;
                deltas.reserve(n - 1);
                for (size_t i = 1; i < n; ++i) {
                    std::chrono::duration<double> diff = window.records[i].timestamp - window.records[i-1].timestamp;
                    deltas.push_back(diff.count());
                }
                
                double sumIA = 0.0;
                for (double d : deltas) sumIA += d;
                meanIA = sumIA / deltas.size();

                double sqSum = 0.0;
                for (double d : deltas) {
                     sqSum += (d - meanIA) * (d - meanIA);
                }
                stdIA = std::sqrt(sqSum / deltas.size());
            }

            double maxAddr = 0.0;
            for (auto const& [k, v] : addrCounts) {
                if (v > maxAddr) maxAddr = v;
            }
            double targetDensity = maxAddr / total;

            fv.values[0] = count;
            fv.values[1] = writeRatio;
            fv.values[2] = funcConc;
            fv.values[3] = entropy;
            fv.values[4] = protoDom;
            fv.values[5] = meanIA;
            fv.values[6] = stdIA;
            fv.values[7] = targetDensity;

            return fv;
        }

    };

}
