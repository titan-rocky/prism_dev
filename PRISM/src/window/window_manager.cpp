#include "prism.hpp"

#include <chrono>
#include <deque>

namespace prism::windowing {

    class WindowManager {
    public:
        WindowManager(std::chrono::milliseconds windowSize, std::chrono::milliseconds stepSize)
            : windowSize_(windowSize), stepSize_(stepSize), initialized_(false) {}

        bool addRecord(const ParsedRecord& record, Window& outWindow) {
            if (initialized_ && record.timestamp < lastTime_) {
                return false; // enforce monotonicity
            }
            lastTime_ = record.timestamp;

            if (!initialized_) {
                nextEmitTime_ = record.timestamp + windowSize_; 
                initialized_ = true;
            }

            buffer_.push_back(record);

            if (record.timestamp >= nextEmitTime_) {
                outWindow.endTime = nextEmitTime_;
                outWindow.startTime = nextEmitTime_ - windowSize_;
                outWindow.records.clear();

                for (const auto& r : buffer_) {
                    if (r.timestamp >= outWindow.startTime && r.timestamp <= outWindow.endTime) {
                        outWindow.records.push_back(r);
                    }
                }

                nextEmitTime_ += stepSize_;

                auto oldestNeeded = nextEmitTime_ - windowSize_;
                
                while (!buffer_.empty() && buffer_.front().timestamp < oldestNeeded) {
                    buffer_.pop_front();
                }

                return true;
            }

            return false;
        }

    private:
        std::chrono::milliseconds windowSize_;
        std::chrono::milliseconds stepSize_;
        std::deque<ParsedRecord> buffer_;
        Timestamp nextEmitTime_;
        Timestamp lastTime_;
        bool initialized_;
    };

} // namespace prism::windowing


