#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <string>
#include <any>
#include <typeindex>

namespace crypto {

// Simple type-erased event bus
class EventBus {
public:
    using Handler = std::function<void(const std::any&)>;

    template<typename EventT>
    void subscribe(std::function<void(const EventT&)> handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_[std::type_index(typeid(EventT))].emplace_back(
            [h = std::move(handler)](const std::any& e) {
                h(std::any_cast<const EventT&>(e));
            });
    }

    template<typename EventT>
    void publish(EventT&& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(std::type_index(typeid(EventT)));
        if (it != handlers_.end()) {
            for (auto& h : it->second) {
                h(std::any(std::forward<EventT>(event)));
            }
        }
    }

    static EventBus& instance() {
        static EventBus bus;
        return bus;
    }

private:
    std::unordered_map<std::type_index, std::vector<Handler>> handlers_;
    std::mutex mutex_;
};

} // namespace crypto
