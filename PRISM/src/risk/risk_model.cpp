#include "prism.hpp"

#include <cmath>
#include <algorithm>

namespace prism::risk {

    class RiskModel {
        public:
            DetectionResult fuse(double hstScore, double burstScore) {
                DetectionResult result;
                result.hstScore = hstScore;
                result.burstScore = burstScore;
                return result;
            }

            RiskDecision evaluate(const DetectionResult& detection) {
                // 1. Normalize (z = score / threshold)
                // We use 10.0 as the 'safe' baseline per configuration
                double z_hst = detection.hstScore / 10.0;
                double z_burst = detection.burstScore / 10.0;

                // 2. Sigmoid Filter p = sigmoid(k(z-1))
                double p_hst = sigmoid(z_hst);
                double p_burst = sigmoid(z_burst);

                // 3. Aggregation (MAX)
                double p_max = std::max(p_hst, p_burst);

                // 4. Binary Escalation with Hysteresis
                if (!isOn_ && p_max >= tauOn_) {
                    isOn_ = true;
                } else if (isOn_ && p_max <= tauOff_) {
                    isOn_ = false;
                }

                RiskDecision decision;
                if (isOn_) {
                    decision.level = RiskLevel::High;
                    decision.escalate = true;
                    decision.restrictWrites = true;
                } else {
                    decision.level = RiskLevel::Low;
                    decision.escalate = false;
                    decision.restrictWrites = false;
                }
                decision.probability = p_max;
                return decision;
            }

        private:
            bool isOn_{false};
            const double tauOn_{0.7};
            const double tauOff_{0.5};
            const double k_{2.0};

            double sigmoid(double z) {
                // p = 1 / (1 + e^(-k(z-1)))
                return 1.0 / (1.0 + std::exp(-k_ * (z - 1.0)));
            }
    };

}
