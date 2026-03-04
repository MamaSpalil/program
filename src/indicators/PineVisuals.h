#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace crypto {

// plotshape() marker from Pine Script
struct PlotShapeMarker {
    double      time{0.0};
    double      price{0.0};
    std::string shape;  // "triangleup"/"triangledown"/"circle"/"cross"
    uint32_t    color{0xFFFFFFFF};
    std::string text;
};

// bgcolor() bar coloring from Pine Script
struct BgColorBar {
    double   time{0.0};
    uint32_t color{0};  // RGBA with low opacity
};

class PineVisuals {
public:
    void addPlotShape(const PlotShapeMarker& marker);
    void addBgColor(const BgColorBar& bar);

    void clearAll();

    std::vector<PlotShapeMarker> getPlotShapes() const;
    std::vector<BgColorBar> getBgColors() const;

private:
    mutable std::mutex mutex_;
    std::vector<PlotShapeMarker> shapes_;
    std::vector<BgColorBar> bgColors_;
};

} // namespace crypto
