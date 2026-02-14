#include "prism.hpp"

#include <functional>
#include <vector>

namespace prism::alert {

    using AlertHandler = std::function<void(const AlertContext&)>;

    class AlertDispatcher {
    public:
        void registerHandler(AlertHandler handler) {
            handlers_.push_back(handler);
        }

        void dispatch(const AlertContext& ctx) {
            for (const auto& handler : handlers_) {
                handler(ctx);
            }
        }

    private:
        std::vector<AlertHandler> handlers_;
    };

}
