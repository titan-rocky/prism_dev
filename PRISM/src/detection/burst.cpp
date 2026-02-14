#include "prism.hpp"

namespace prism::detection {

    class BurstDetector {
        public:
            // Tunable Baseline for Inter-Arrival Variance (e.g., standard polling jitter)
            const double BASE_SIGMA = 0.05; 
            const double EPSILON = 0.001;

            double score(const FeatureVector& fv) {
                if (fv.values.size() < 8) return 0.0;

                double currentSigma = fv.values[6];
                
                double varCurrent = currentSigma * currentSigma;
                double varBase = BASE_SIGMA * BASE_SIGMA;

                // B_z = |sigma^2 - sigma_base^2| / (epsilon + sigma_base)
                double numerator = std::abs(varCurrent - varBase);
                double denominator = EPSILON + BASE_SIGMA;

                return numerator / denominator;
            }

    };

}
