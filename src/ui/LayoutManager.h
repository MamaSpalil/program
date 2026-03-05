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
struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}
};
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
        bool        visible{true};
    };

    // Apply position and size every frame BEFORE ImGui::Begin() — locks window
    static void lockWindow(const char* name, Vec2 pos, Vec2 size) {
#ifdef IMGUI_VERSION
        ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_Always);
#else
        (void)name; (void)pos; (void)size;
#endif
    }

    // Apply position and size only on first use — allows imgui.ini persistence
    static void lockWindowOnce(const char* name, Vec2 pos, Vec2 size) {
#ifdef IMGUI_VERSION
        ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_FirstUseEver);
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

    // Recalculate all window layouts based on viewport dimensions.
    // Produces 8 windows:
    //   Main Toolbar, Filters Bar, Pair List, Volume Delta,
    //   Market Data, Indicators, User Panel, Logs
    //
    // offsetX/offsetY: viewport WorkPos (global origin of work area)
    // show* flags: when false, the panel's area collapses to zero and
    //              neighbours expand to fill the freed space.
    void recalculate(float screenW, float screenH,
                     float offsetX = 0.0f, float offsetY = 0.0f,
                     bool showPairList   = true,
                     bool showUserPanel  = true,
                     bool showVolumeDelta = true,
                     bool showIndicators = true,
                     bool showLogs       = true);

    // Layout height proportions for center column
    // (vdPct + indPct < 1.0; Market Data gets the remainder)
    float logPct() const  { return logPct_; }
    float vdPct()  const  { return vdPct_;  }
    float indPct() const  { return indPct_; }

    void setLogPct(float v)  { logPct_ = v; }
    void setVdPct(float v)   { vdPct_  = v; }
    void setIndPct(float v)  { indPct_ = v; }

    // Get the layout for a specific window by name
    WindowLayout get(const std::string& name) const {
        auto it = layouts_.find(name);
        if (it != layouts_.end()) return it->second;
        return {name, Vec2(100, 100), Vec2(400, 300), true};
    }

    bool hasLayout(const std::string& name) const {
        return layouts_.find(name) != layouts_.end();
    }

private:
    std::map<std::string, WindowLayout> layouts_;
    float logPct_ = 0.10f;
    float vdPct_  = 0.15f;
    float indPct_ = 0.20f;
};

} // namespace crypto
