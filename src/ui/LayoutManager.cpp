#include "LayoutManager.h"
#include <algorithm>
#include <cmath>

namespace crypto {

void LayoutManager::recalculate(float screenW, float screenH,
                                 float offsetX, float offsetY,
                                 bool showPairList, bool showUserPanel,
                                 bool showVolumeDelta, bool showIndicators,
                                 bool showLogs) {
    layouts_.clear();

    // Viewport origin & size
    const float Xvp = offsetX;
    const float Yvp = offsetY;
    const float Wvp = std::max(0.0f, screenW);
    const float Hvp = std::max(0.0f, screenH);

    // Fixed constants
    const float Htop     = 32.0f;   // Main Toolbar height
    const float Hfilters = 28.0f;   // Filters Bar height

    // Effective dimensions based on show flags
    const float Wleft  = showPairList  ? 210.0f : 0.0f;
    const float Wright = showUserPanel ? 290.0f : 0.0f;
    const float Hlogs  = showLogs      ? 120.0f : 0.0f;

    // Dynamic center column dimensions
    const float Wcenter = std::max(0.0f, Wvp - Wleft - Wright);
    const float Hcenter = std::max(0.0f, Hvp - Htop - Hfilters - Hlogs);

    // Center column height distribution using floor for integer pixel alignment
    const float Hvd  = showVolumeDelta ? std::floor(Hcenter * vdPct_)  : 0.0f;
    const float Hind = showIndicators  ? std::floor(Hcenter * indPct_) : 0.0f;
    const float Hmarket = std::max(0.0f, Hcenter - Hvd - Hind);

    // Y-base for the center row (below toolbar + filters)
    const float Ybase   = Yvp + Htop + Hfilters;
    const float Xcenter = Xvp + Wleft;
    const float Xright  = Xvp + Wvp - Wright;

    // 1. Main Toolbar — full width, top
    layouts_["Main Toolbar"] = {
        "Main Toolbar",
        Vec2(Xvp, Yvp),
        Vec2(std::max(0.0f, Wvp), Htop),
        true   // always visible
    };

    // 2. Filters Bar — full width, below toolbar
    layouts_["Filters Bar"] = {
        "Filters Bar",
        Vec2(Xvp, Yvp + Htop),
        Vec2(std::max(0.0f, Wvp), Hfilters),
        true   // always visible
    };

    // 3. Pair List — left sidebar
    layouts_["Pair List"] = {
        "Pair List",
        Vec2(Xvp, Ybase),
        Vec2(std::max(0.0f, Wleft), std::max(0.0f, Hcenter)),
        showPairList
    };

    // 4. User Panel — right sidebar
    layouts_["User Panel"] = {
        "User Panel",
        Vec2(Xright, Ybase),
        Vec2(std::max(0.0f, Wright), std::max(0.0f, Hcenter)),
        showUserPanel
    };

    // 5. Volume Delta — center column, top
    layouts_["Volume Delta"] = {
        "Volume Delta",
        Vec2(Xcenter, Ybase),
        Vec2(std::max(0.0f, Wcenter), std::max(0.0f, Hvd)),
        showVolumeDelta
    };

    // 6. Market Data — center column, middle (gets remaining height)
    layouts_["Market Data"] = {
        "Market Data",
        Vec2(Xcenter, Ybase + Hvd),
        Vec2(std::max(0.0f, Wcenter), std::max(0.0f, Hmarket)),
        true   // always visible
    };

    // 7. Indicators — center column, bottom
    layouts_["Indicators"] = {
        "Indicators",
        Vec2(Xcenter, Ybase + Hvd + Hmarket),
        Vec2(std::max(0.0f, Wcenter), std::max(0.0f, Hind)),
        showIndicators
    };

    // 8. Logs — bottom, full width
    layouts_["Logs"] = {
        "Logs",
        Vec2(Xvp, Yvp + Hvp - Hlogs),
        Vec2(std::max(0.0f, Wvp), std::max(0.0f, Hlogs)),
        showLogs
    };
}

} // namespace crypto
