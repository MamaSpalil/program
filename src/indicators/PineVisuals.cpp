#include "PineVisuals.h"

namespace crypto {

void PineVisuals::addPlotShape(const PlotShapeMarker& marker) {
    std::lock_guard<std::mutex> lk(mutex_);
    shapes_.push_back(marker);
}

void PineVisuals::addBgColor(const BgColorBar& bar) {
    std::lock_guard<std::mutex> lk(mutex_);
    bgColors_.push_back(bar);
}

void PineVisuals::clearAll() {
    std::lock_guard<std::mutex> lk(mutex_);
    shapes_.clear();
    bgColors_.clear();
}

std::vector<PlotShapeMarker> PineVisuals::getPlotShapes() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return shapes_;
}

std::vector<BgColorBar> PineVisuals::getBgColors() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return bgColors_;
}

} // namespace crypto
