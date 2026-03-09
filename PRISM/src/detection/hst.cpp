#include "prism.hpp"

#include <cmath>

namespace prism::detection {

    class HalfSpaceTrees {
        public:
            double score(const FeatureVector& fv) {
                if (fv.values.size() < 8) {
                    return 0.0;
                }

                double writeRisk = fv.values[1] * 50.0;
                double funcRisk = fv.values[2] * 2.5;
                double sprayRisk = fv.values[3] * fv.values[1] * 20.0;

                return writeRisk + funcRisk + sprayRisk;
            }
    };

}
