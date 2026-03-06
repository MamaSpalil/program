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
    const float gap      = 1.0f;    // Pixel gap between adjacent windows

    // Left column width: shown when either PairList or VD is visible
    const bool  showLeft = showPairList || showVolumeDelta;
    const float Wleft  = showLeft      ? 200.0f : 0.0f;
    const float Wright = showUserPanel ? 290.0f : 0.0f;
    // Logs height: percentage-based with minimum constraint (min 40px)
    const float Hlogs  = showLogs
        ? std::max(40.0f, std::floor(Hvp * logPct_))
        : 0.0f;

    // Gaps between columns and rows
    const float gapLR  = (showLeft ? gap : 0.0f);    // gap between left column and center
    const float gapRC  = (showUserPanel ? gap : 0.0f); // gap between center and right column
    const float gapLog = (showLogs ? gap : 0.0f);     // gap above logs

    // Dynamic center column dimensions (account for gaps)
    const float Wcenter = std::max(0.0f, Wvp - Wleft - Wright - gapLR - gapRC);
    const float Hcenter = std::max(0.0f, Hvp - Htop - Hfilters - Hlogs - gapLog);

    // Left column height distribution: Pair List (top) + Volume Delta (bottom)
    const float gapVD = (showVolumeDelta && showPairList && showLeft) ? gap : 0.0f;
    const float Hvd = (showVolumeDelta && showLeft) ? std::floor(Hcenter * vdPct_) : 0.0f;
    const float HpairList = std::max(0.0f, Hcenter - Hvd - gapVD);

    // Center column height distribution: Market Data (top) + Indicators (bottom)
    const float gapInd = showIndicators ? gap : 0.0f;
    const float Hind = showIndicators ? std::floor(Hcenter * indPct_) : 0.0f;
    const float Hmarket = std::max(0.0f, Hcenter - Hind - gapInd);

    // Y-base for the content area (below toolbar + filters)
    const float Ybase   = Yvp + Htop + Hfilters;
    const float Xcenter = Xvp + Wleft + gapLR;
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

    // 3. Pair List — left column, top
    layouts_["Pair List"] = {
        "Pair List",
        Vec2(Xvp, Ybase),
        Vec2(std::max(0.0f, Wleft), std::max(0.0f, HpairList)),
        showPairList
    };

    // 4. Volume Delta — left column, below Pair List (with gap)
    layouts_["Volume Delta"] = {
        "Volume Delta",
        Vec2(Xvp, Ybase + HpairList + gapVD),
        Vec2(std::max(0.0f, Wleft), std::max(0.0f, Hvd)),
        showVolumeDelta
    };

    // 5. Market Data — center column, top (gets remaining height)
    layouts_["Market Data"] = {
        "Market Data",
        Vec2(Xcenter, Ybase),
        Vec2(std::max(0.0f, Wcenter), std::max(0.0f, Hmarket)),
        true   // always visible
    };

    // 6. Indicators — center column, below Market Data (with gap)
    layouts_["Indicators"] = {
        "Indicators",
        Vec2(Xcenter, Ybase + Hmarket + gapInd),
        Vec2(std::max(0.0f, Wcenter), std::max(0.0f, Hind)),
        showIndicators
    };

    // 7. User Panel — right sidebar (full center height)
    layouts_["User Panel"] = {
        "User Panel",
        Vec2(Xright, Ybase),
        Vec2(std::max(0.0f, Wright), std::max(0.0f, Hcenter)),
        showUserPanel
    };

    // 8. Logs — bottom, full width (with gap above)
    layouts_["Logs"] = {
        "Logs",
        Vec2(Xvp, Yvp + Hvp - Hlogs),
        Vec2(std::max(0.0f, Wvp), std::max(0.0f, Hlogs)),
        showLogs
    };
}

} // namespace crypto
