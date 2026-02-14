#include "prism.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace prism::utils {

    Logger& Logger::instance() {
        static Logger logger;
        return logger;
    }

    void Logger::info(const std::string& msg) {
        // implementation uses the private log body
        const std::string level = "INFO";
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream os;
        os << std::put_time(&tm, "%F %T");
        std::cerr << "[" << os.str() << "] " << level << ": " << msg << '\n';
    }

    void Logger::warn(const std::string& msg) {
        const std::string level = "WARN";
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream os;
        os << std::put_time(&tm, "%F %T");
        std::cerr << "[" << os.str() << "] " << level << ": " << msg << '\n';
    }

    void Logger::error(const std::string& msg) {
        const std::string level = "ERROR";
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream os;
        os << std::put_time(&tm, "%F %T");
        std::cerr << "[" << os.str() << "] " << level << ": " << msg << '\n';
    }

}
