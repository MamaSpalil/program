#include "LayoutManager.h"

namespace crypto {

void LayoutManager::recalculate(float screenW, float screenH) {
    layouts_.clear();

    // Fixed dimensions from layout spec
    const float toolbarH   = 32.0f;
    const float filtersH   = 28.0f;
    const float logsH      = 120.0f;
    const float pairListW  = 210.0f;
    const float userPanelW = 290.0f;

    const float centerW = screenW - pairListW - userPanelW;
    const float centerH = screenH - toolbarH - filtersH - logsH;
    const float centerTop = toolbarH + filtersH;  // = 60

    // Main Toolbar — full width, top
    layouts_["Main Toolbar"] = {
        "Main Toolbar",
        Vec2(0, 0),
        Vec2(screenW, toolbarH),
        true
    };

    // Filters Bar — full width, below toolbar
    layouts_["Filters Bar"] = {
        "Filters Bar",
        Vec2(0, toolbarH),
        Vec2(screenW, filtersH),
        true
    };

    // Pair List — left sidebar
    layouts_["Pair List"] = {
        "Pair List",
        Vec2(0, centerTop),
        Vec2(pairListW, centerH),
        true
    };

    // Center column: Volume Delta, Market Data, Indicators
    // Use configurable proportions (vdPct_, indPct_; Market Data gets remainder)
    float vdH  = centerH * vdPct_;
    float indH = centerH * indPct_;
    float mdH  = centerH - vdH - indH;

    // Enforce minimums
    if (vdH  < 40.0f)  vdH  = 40.0f;
    if (indH < 40.0f)  indH = 40.0f;
    if (mdH  < 100.0f) mdH  = 100.0f;

    float volY = centerTop;

    // Volume Delta — center column, top
    layouts_["Volume Delta"] = {
        "Volume Delta",
        Vec2(pairListW, volY),
        Vec2(centerW, vdH),
        true
    };

    float marketY = volY + vdH;

    // Market Data — center column, middle (tallest)
    layouts_["Market Data"] = {
        "Market Data",
        Vec2(pairListW, marketY),
        Vec2(centerW, mdH),
        true
    };

    float indY = marketY + mdH;

    // Indicators — center column, bottom
    layouts_["Indicators"] = {
        "Indicators",
        Vec2(pairListW, indY),
        Vec2(centerW, indH),
        true
    };

    // User Panel — right sidebar
    layouts_["User Panel"] = {
        "User Panel",
        Vec2(screenW - userPanelW, centerTop),
        Vec2(userPanelW, centerH),
        true
    };

    // Logs — bottom, full width
    layouts_["Logs"] = {
        "Logs",
        Vec2(0, screenH - logsH),
        Vec2(screenW, logsH),
        true
    };
}

} // namespace crypto
