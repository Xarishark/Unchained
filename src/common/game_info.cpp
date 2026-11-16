#include "app/app.h"
#include "core/input.h"
#include "core/log.h"

#include <algorithm>
#include <cstdlib>
#include <windows.h>

#include <toml++/toml.hpp>

namespace {

// UNCHAINED_OFFSCREEN set: the game window lives far off-screen, for automated boot tests (screenshots still work
// through PrintWindow). The bring-up builds can strobe between clear colours, so test runs never show on a monitor.
bool Offscreen() {
    static const bool on = std::getenv("UNCHAINED_OFFSCREEN") != nullptr;
    return on;
}

constexpr int kAway = -32000;

void OffscreenWindow(void* hwnd) {
    if (Offscreen())
        SetWindowPos(static_cast<HWND>(hwnd), nullptr, kAway, kAway, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

bool OffscreenMessage(void*, uint32_t msg, uintptr_t, intptr_t lp, intptr_t*) {
    if (Offscreen() && msg == WM_WINDOWPOSCHANGING) {
        auto* pos = reinterpret_cast<WINDOWPOS*>(lp);
        pos->x = kAway;
        pos->y = kAway;
        pos->flags &= ~UINT(SWP_NOMOVE);
        pos->flags |= SWP_NOACTIVATE;
    }
    return false;  // the host handles it as usual
}

// shadow_resolution: the pixels across the game's 256x256 shadow map (the pushed colour target at 0xD3E980), 0 to
// follow the render scale like every other target
uint32_t g_shadow_resolution = 2048;
constexpr uint32_t kShadowMap = 0x00D3E980;
constexpr uint32_t kShadowSize = 256;
constexpr uint32_t kShadowMax = 8192;

float TargetScale(uint32_t data, uint32_t gw, uint32_t gh, bool depth, float render_scale) {
    if (depth || (data & 0x0FFFFFFF) != kShadowMap || gw != kShadowSize || gh != kShadowSize) return 0;
    if (!g_shadow_resolution) return 0;
    // never coarser than the render scale would draw it
    uint32_t px = (std::min)(g_shadow_resolution, kShadowMax);
    float sc = (std::max)(float(px) / kShadowSize, render_scale);
    LOG_FIRST(1, "unchained: shadow map at %ux%u", uint32_t(kShadowSize * sc), uint32_t(kShadowSize * sc));
    return sc;
}

// defaults that differ from the SDK's, for a settings file that does not name them yet
constexpr uint32_t kDefaultAniso = 16;
bool g_aniso_named = false;

void LoadOptions(const void* table) {
    g_aniso_named = static_cast<const toml::table*>(table)->contains("anisotropic_filtering");
}

void InitOptions() {
    if (!g_aniso_named) app::Get().aniso = kDefaultAniso;
}

}  // namespace

void UnchainedPad(int port, input::Pad& p);  // input_keys.cpp: keyboard and scripted test input
extern uint32_t g_fov_degrees;  // widescreen.cpp
// controls.cpp: camera speed and inversion, vibration strength
extern uint32_t g_camera_speed, g_vibration;
extern bool g_invert_camera_x, g_invert_camera_y;
void ControlsPad(int port, input::Pad& p);
void ControlsRumble(int port, uint16_t& left, uint16_t& right);

// what the runtime asks of the game; strings must be set, hooks may stay null
const app::GameInfo app::kGame = {
    .title = L"Unchained",                // window and splash
    .short_title = L"Unchained",          // message boxes
    .name = L"Unchained",                 // window classes, crash dumps, handover event
    .log = "unchained",                   // log file prefix
    .settings = "settings.toml",          // settings file beside the exe
    .splash = L"splash",                  // optional splash.png or splash.bmp beside the exe
    .missing = L"Put the game in the assets folder: its disc image (.iso), or default.xbe with its folders beside it.",
    .images = {{"default.xbe", "Unchained"}},  // each recompiled xbe and its exe, for title launches
    .options = {{"shadow_resolution", nullptr, &g_shadow_resolution},
                {"fov", nullptr, &g_fov_degrees},
                {"camera_speed", nullptr, &g_camera_speed},
                {"invert_camera_x", &g_invert_camera_x, nullptr},
                {"invert_camera_y", &g_invert_camera_y, nullptr},
                {"vibration", nullptr, &g_vibration}},  // game settings kept in the settings file  // game settings kept in the settings file
    .load = LoadOptions,                         // reads more from the settings file's toml::table
    .save = nullptr,                      // writes more into it
    .init = InitOptions,                         // after the settings load, before the window and boot
    .loaded = nullptr,                    // image in memory, before its entry point runs
    .window = OffscreenWindow,                    // the game window is up
    .message = OffscreenMessage,                  // window messages first; true takes one
    .ui = nullptr,                        // ImGui drawing each frame
    .pad = ControlsPad,                  // keyboard and scripted input on port 0 (input_keys.cpp)
    .rumble = ControlsRumble,                    // changes a rumble before it is set
    .releases = {},                       // one release: the retail xbe
    .view_w = 640,                        // the game draws 640x448 of its 640x480 back buffer (viewport c-96/c-95)
    .view_h = 448,
    .expands = true,                      // src/game/widescreen.cpp widens its projection
    .target_scale = TargetScale,          // the shadow map at shadow_resolution
};
