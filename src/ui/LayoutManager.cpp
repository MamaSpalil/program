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

    // Gaps between columns
    const float gapLR  = (showLeft ? gap : 0.0f);      // gap between left column and center
    const float gapRC  = (showUserPanel ? gap : 0.0f);  // gap between center and right column

    // Full content area height (below toolbar + filters)
    const float Hcontent = std::max(0.0f, Hvp - Htop - Hfilters);

    // Dynamic center column width (account for column gaps)
    const float Wcenter = std::max(0.0f, Wvp - Wleft - Wright - gapLR - gapRC);

    // Left column height distribution: Pair List (top) + Volume Delta (bottom)
    // Left column spans full content height
    const float gapVD = (showVolumeDelta && showPairList && showLeft) ? gap : 0.0f;
    const float Hvd = (showVolumeDelta && showLeft) ? std::floor(Hcontent * vdPct_) : 0.0f;
    const float HpairList = std::max(0.0f, Hcontent - Hvd - gapVD);

    // Center column height distribution (v2.7.0 layout):
    //   Indicators (top) → Market Data (middle) → Logs (bottom)
    const float gapInd  = showIndicators ? gap : 0.0f;  // gap below indicators
    const float gapLog  = showLogs       ? gap : 0.0f;  // gap above logs
    const float Hind    = showIndicators ? std::floor(Hcontent * indPct_) : 0.0f;
    const float Hlogs   = showLogs
        ? std::max(40.0f, std::floor(Hcontent * logPct_))
        : 0.0f;
    const float Hmarket = std::max(0.0f, Hcontent - Hind - Hlogs - gapInd - gapLog);

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

    // 3. Pair List — left column, top (full left height minus VD)
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

    // 5. Indicators — center column, top (v2.7.0: moved above Market Data)
    layouts_["Indicators"] = {
        "Indicators",
        Vec2(Xcenter, Ybase),
        Vec2(std::max(0.0f, Wcenter), std::max(0.0f, Hind)),
        showIndicators
    };

    // 6. Market Data — center column, below Indicators (gets remaining height)
    layouts_["Market Data"] = {
        "Market Data",
        Vec2(Xcenter, Ybase + Hind + gapInd),
        Vec2(std::max(0.0f, Wcenter), std::max(0.0f, Hmarket)),
        true   // always visible
    };

    // 7. Logs — center column, below Market Data (v2.7.0: center width, not full)
    layouts_["Logs"] = {
        "Logs",
        Vec2(Xcenter, Ybase + Hind + gapInd + Hmarket + gapLog),
        Vec2(std::max(0.0f, Wcenter), std::max(0.0f, Hlogs)),
        showLogs
    };

    // 8. User Panel — right sidebar (full content height)
    layouts_["User Panel"] = {
        "User Panel",
        Vec2(Xright, Ybase),
        Vec2(std::max(0.0f, Wright), std::max(0.0f, Hcontent)),
        showUserPanel
    };
}

} // namespace crypto
