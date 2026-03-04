#pragma once

#include <string>
#include <map>

// When compiling with the GUI (imgui available), use real ImGui types.
// When compiling without (tests), provide lightweight stubs.
#ifdef IMGUI_VERSION
// Already included — use real ImVec2 and ImGuiWindowFlags
#else

// Check if imgui.h was NOT already included via a forward-declare guard
#ifndef LAYOUT_MANAGER_IMGUI_STUB
#define LAYOUT_MANAGER_IMGUI_STUB

// Lightweight stub used only in non-GUI test builds
namespace layout_detail {
struct Vec2 { float x, y; Vec2() : x(0), y(0) {} Vec2(float _x, float _y) : x(_x), y(_y) {} };
enum StubFlags {
    SF_NoMove                = 1 << 1,
    SF_NoResize              = 1 << 2,
    SF_NoCollapse            = 1 << 5,
    SF_HorizontalScrollbar   = 1 << 15,
    SF_NoBringToDisplayFront = 1 << 18
};
} // namespace layout_detail

#endif // LAYOUT_MANAGER_IMGUI_STUB
#endif // IMGUI_VERSION

namespace crypto {

class LayoutManager {
public:
#ifdef IMGUI_VERSION
    using Vec2 = ImVec2;
#else
    using Vec2 = layout_detail::Vec2;
#endif

    struct WindowLayout {
        std::string name;
        Vec2        pos;
        Vec2        size;
    };

    // Apply position and size every frame BEFORE ImGui::Begin()
    static void lockWindow(const char* name, Vec2 pos, Vec2 size) {
#ifdef IMGUI_VERSION
        ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_Always);
#else
        (void)name; (void)pos; (void)size;
#endif
    }

    // Flags for a fixed window (no move, no resize, no collapse)
    static int lockedFlags() {
#ifdef IMGUI_VERSION
        return ImGuiWindowFlags_NoMove
             | ImGuiWindowFlags_NoResize
             | ImGuiWindowFlags_NoCollapse
             | ImGuiWindowFlags_NoBringToFrontOnFocus;
#else
        return layout_detail::SF_NoMove | layout_detail::SF_NoResize
             | layout_detail::SF_NoCollapse | layout_detail::SF_NoBringToDisplayFront;
#endif
    }

    // Flags for a fixed window with horizontal scrollbar
    static int lockedScrollFlags() {
#ifdef IMGUI_VERSION
        return ImGuiWindowFlags_NoMove
             | ImGuiWindowFlags_NoResize
             | ImGuiWindowFlags_NoCollapse
             | ImGuiWindowFlags_NoBringToFrontOnFocus
             | ImGuiWindowFlags_HorizontalScrollbar;
#else
        return layout_detail::SF_NoMove | layout_detail::SF_NoResize
             | layout_detail::SF_NoCollapse | layout_detail::SF_NoBringToDisplayFront
             | layout_detail::SF_HorizontalScrollbar;
#endif
    }

    // Recalculate all window layouts based on screen dimensions
    void recalculate(float screenW, float screenH) {
        layouts_.clear();

        float pairListW = 200.0f;

        // Volume Delta — top area, right of pair list
        layouts_["Volume Delta"] = {
            "Volume Delta",
            Vec2(pairListW + 4.0f, 30.0f),
            Vec2(screenW - pairListW - 8.0f, 150.0f)
        };

        // Market Data — below Volume Delta
        float mdY = 30.0f + 150.0f + 4.0f;
        float mdH = screenH - mdY - 260.0f - 30.0f;
        if (mdH < 300.0f) mdH = 300.0f;
        layouts_["Market Data"] = {
            "Market Data",
            Vec2(pairListW + 4.0f, mdY),
            Vec2(screenW - pairListW - 8.0f, mdH)
        };

        // Indicators — below Market Data
        float indY = mdY + mdH + 4.0f;
        float indH = screenH - indY - 30.0f;
        if (indH < 100.0f) indH = 100.0f;
        layouts_["Indicators"] = {
            "Indicators",
            Vec2(pairListW + 4.0f, indY),
            Vec2(screenW - pairListW - 8.0f, indH)
        };
    }

    // Get the layout for a specific window by name
    WindowLayout get(const std::string& name) const {
        auto it = layouts_.find(name);
        if (it != layouts_.end()) return it->second;
        return {name, Vec2(100, 100), Vec2(400, 300)};
    }

    bool hasLayout(const std::string& name) const {
        return layouts_.find(name) != layouts_.end();
    }

private:
    std::map<std::string, WindowLayout> layouts_;
};

} // namespace crypto
