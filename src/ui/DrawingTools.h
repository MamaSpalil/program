#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <nlohmann/json.hpp>

namespace crypto { class DrawingRepository; }

namespace crypto {

enum class DrawingType {
    TREND_LINE,
    HORIZONTAL_LINE,
    FIBONACCI,
    RECTANGLE,
    TEXT_LABEL
};

struct DrawingObject {
    std::string  id;
    DrawingType  type{DrawingType::TREND_LINE};
    double       x1{0.0}, y1{0.0};
    double       x2{0.0}, y2{0.0};
    uint32_t     color{0xFFFFFFFF};
    float        thickness{1.0f};
    std::string  text;
};

class DrawingManager {
public:
    DrawingManager() = default;

    void add(const DrawingObject& obj);
    void remove(const std::string& id);
    int  count() const;
    std::vector<DrawingObject> getAll() const;

    void save(const std::string& symbol,
              const std::string& dir = "config/drawings") const;
    void load(const std::string& symbol,
              const std::string& dir = "config/drawings");
    void clear();

    void setRepository(DrawingRepository* repo) { drawRepo_ = repo; }
    void setExchange(const std::string& exchange) { exchange_ = exchange; }

private:
    std::vector<DrawingObject> objects_;
    mutable std::mutex mutex_;
    DrawingRepository* drawRepo_{nullptr};
    std::string exchange_;
};

} // namespace crypto
