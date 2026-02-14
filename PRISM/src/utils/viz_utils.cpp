#include <string>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace prism::utils {

    inline std::string getFuzzyLabel(double p) {
        if (p < 0.2) return "SAFE";
        if (p < 0.5) return "SUSPICIOUS";
        if (p < 0.8) return "THREAT";
        return "CRITICAL";
    }

    inline std::string getProgressBar(double p, int width = 20) {
        int fill = static_cast<int>(p * width);
        if (fill > width) fill = width;
        if (fill < 0) fill = 0;

        std::ostringstream ss;
        ss << "[";
        for (int i = 0; i < fill; ++i) ss << "#";
        for (int i = fill; i < width; ++i) ss << ".";
        ss << "]";
        return ss.str();
    }

}
