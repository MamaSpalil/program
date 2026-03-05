#include "DrawingTools.h"
#include "../data/DrawingRepository.h"
#include <algorithm>
#include <fstream>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace crypto {

void DrawingManager::add(const DrawingObject& obj) {
    std::lock_guard<std::mutex> lk(mutex_);
    objects_.push_back(obj);
    if (drawRepo_) drawRepo_->insert(obj, exchange_);
}

void DrawingManager::remove(const std::string& id) {
    std::lock_guard<std::mutex> lk(mutex_);
    objects_.erase(
        std::remove_if(objects_.begin(), objects_.end(),
            [&](const DrawingObject& o) { return o.id == id; }),
        objects_.end());
    if (drawRepo_) drawRepo_->remove(id);
}

int DrawingManager::count() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<int>(objects_.size());
}

std::vector<DrawingObject> DrawingManager::getAll() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return objects_;
}

void DrawingManager::clear() {
    std::lock_guard<std::mutex> lk(mutex_);
    objects_.clear();
}

static std::string drawingTypeToStr(DrawingType t) {
    switch (t) {
        case DrawingType::TREND_LINE:       return "TREND_LINE";
        case DrawingType::HORIZONTAL_LINE:  return "HORIZONTAL_LINE";
        case DrawingType::FIBONACCI:        return "FIBONACCI";
        case DrawingType::RECTANGLE:        return "RECTANGLE";
        case DrawingType::TEXT_LABEL:       return "TEXT_LABEL";
    }
    return "TREND_LINE";
}

static DrawingType strToDrawingType(const std::string& s) {
    if (s == "HORIZONTAL_LINE") return DrawingType::HORIZONTAL_LINE;
    if (s == "FIBONACCI")       return DrawingType::FIBONACCI;
    if (s == "RECTANGLE")       return DrawingType::RECTANGLE;
    if (s == "TEXT_LABEL")      return DrawingType::TEXT_LABEL;
    return DrawingType::TREND_LINE;
}

void DrawingManager::save(const std::string& symbol,
                           const std::string& dir) const {
    std::lock_guard<std::mutex> lk(mutex_);
#ifdef _WIN32
    _mkdir(dir.c_str());
#else
    mkdir(dir.c_str(), 0755);
#endif
    std::string path = dir + "/" + symbol + "_drawings.json";
    nlohmann::json arr = nlohmann::json::array();
    for (auto& o : objects_) {
        arr.push_back({
            {"id", o.id},
            {"type", drawingTypeToStr(o.type)},
            {"x1", o.x1}, {"y1", o.y1},
            {"x2", o.x2}, {"y2", o.y2},
            {"color", o.color},
            {"thickness", o.thickness},
            {"text", o.text}
        });
    }
    std::ofstream f(path);
    if (f.is_open()) f << arr.dump(2);
}

void DrawingManager::load(const std::string& symbol,
                           const std::string& dir) {
    std::string path = dir + "/" + symbol + "_drawings.json";
    std::ifstream f(path);
    if (!f.is_open()) return;
    try {
        auto arr = nlohmann::json::parse(f);
        std::lock_guard<std::mutex> lk(mutex_);
        objects_.clear();
        for (auto& j : arr) {
            DrawingObject o;
            o.id        = j.value("id", "");
            o.type      = strToDrawingType(j.value("type", ""));
            o.x1        = j.value("x1", 0.0);
            o.y1        = j.value("y1", 0.0);
            o.x2        = j.value("x2", 0.0);
            o.y2        = j.value("y2", 0.0);
            o.color     = j.value("color", 0xFFFFFFFFu);
            o.thickness = j.value("thickness", 1.0f);
            o.text      = j.value("text", "");
            objects_.push_back(o);
        }
    } catch (...) {}
}

} // namespace crypto
