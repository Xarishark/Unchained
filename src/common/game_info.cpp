#include "app/app.h"
#include "core/input.h"
#include "core/log.h"
#include "common/version.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <windows.h>

#include <toml++/toml.hpp>

// overlay.cpp: the performance overlay (F10)
bool OverlayMessage(uint32_t msg, uintptr_t wp);
void OverlayUi(double fps);
extern bool g_perf_overlay;

// keyboard.cpp: rebindable keys in the settings file, the hidden cursor of the mouse camera
void LoadKeys(const void* table);
void SaveKeys(void* table);
bool KeyboardMessage(void* hwnd, uint32_t msg, uintptr_t wp, intptr_t lp, intptr_t* result);

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

bool OffscreenMessage(void* hwnd, uint32_t msg, uintptr_t wp, intptr_t lp, intptr_t* result) {
    if (KeyboardMessage(hwnd, msg, wp, lp, result)) return true;
    if (OverlayMessage(msg, wp)) {
        *result = 0;
        return true;
    }
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

// character_shadows false: the draws that lay the shadow map onto the floor are left out (the map itself is still
// drawn, unseen)
bool g_character_shadows = true;
bool SkipTexture(uint32_t data) {
    bool skip = !g_character_shadows && (data & 0x0FFFFFFF) == kShadowMap;
    if (skip) LOG_FIRST(1, "unchained: character shadows off: the shadow map's floor draws are left out");
    return skip;
}

// defaults that differ from the SDK's, for a settings file that does not name them yet
constexpr uint32_t kDefaultAniso = 16;
bool g_aniso_named = false;

// graphics_preset: "custom" leaves each setting as the file has it; the others set the graphics ones together
// (the Configuration app writes the preset's name with them)
struct Preset {
    const char* name;
    uint32_t render_percent, aniso, shadow, upscaler, sharpen, hud_edge;
    app::Wide widescreen;  // Expand stays the player's choice (Console, Off, Expand) unless the preset is the console's
    bool console_look;
};
constexpr Preset kPresets[] = {
    {"original", 100, 0, 0, 0, 0, 0, app::Wide::Off, true},  // the console's picture, sharp at the output's size
    {"low", 75, 4, 1024, 1, 30, 100, app::Wide::Expand, false},
    {"high", 100, 16, 2048, 1, 0, 100, app::Wide::Expand, false},
    {"max", 200, 16, 4096, 1, 0, 100, app::Wide::Expand, false},
};
std::string g_preset = "custom";

void ApplyPreset() {
    for (const Preset& p : kPresets) {
        if (g_preset != p.name) continue;
        app::Settings& s = app::Get();
        s.render_percent = p.render_percent;
        s.aniso = p.aniso;
        s.upscaler = p.upscaler;
        s.sharpen = p.sharpen;
        s.hud_edge = p.hud_edge;
        if (p.console_look) s.widescreen = p.widescreen;
        g_shadow_resolution = p.shadow;
        LOGI("unchained: graphics preset %s", p.name);
    }
}

void LoadOptions(const void* table) {
    LoadKeys(table);
    const auto& t = *static_cast<const toml::table*>(table);
    g_aniso_named = t.contains("anisotropic_filtering");
    g_preset = t["graphics_preset"].value_or(g_preset);
}

void SaveOptions(void* table) {
    static_cast<toml::table*>(table)->insert_or_assign("graphics_preset", g_preset);
    SaveKeys(table);
}

void InitOptions() {
    LOGR("Unchained %s", kUnchainedVersion);
    if (!g_aniso_named) app::Get().aniso = kDefaultAniso;
    ApplyPreset();
}

}  // namespace

// the Configuration app (src/launcher): the preset it sets and applies
std::string& GraphicsPreset() { return g_preset; }
void ApplyGraphicsPreset() { ApplyPreset(); }
bool UnchainedLauncher();  // src/launcher/launcher.cpp

void UnchainedPad(int port, input::Pad& p);  // input_keys.cpp: keyboard and scripted test input
extern uint32_t g_fov_degrees, g_draw_distance;  // widescreen.cpp
// controls.cpp: camera speed and inversion, vibration strength
extern uint32_t g_camera_speed, g_vibration;
extern bool g_invert_camera_x, g_invert_camera_y;
extern bool g_mouse_camera;  // keyboard.cpp
extern uint32_t g_mouse_sensitivity;
void ControlsPad(int port, input::Pad& p);
extern uint32_t g_button_prompts;  // prompts.cpp
bool PromptsEdits(uint64_t key);
bool PromptsImage(uint64_t key, uint32_t& w, uint32_t& h, std::vector<uint8_t>& bgra);
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
                {"draw_distance", nullptr, &g_draw_distance},
                {"camera_speed", nullptr, &g_camera_speed},
                {"invert_camera_x", &g_invert_camera_x, nullptr},
                {"invert_camera_y", &g_invert_camera_y, nullptr},
                {"vibration", nullptr, &g_vibration},
                {"mouse_camera", &g_mouse_camera, nullptr},
                {"mouse_sensitivity", nullptr, &g_mouse_sensitivity},
                {"character_shadows", &g_character_shadows, nullptr},
                {"perf_overlay", &g_perf_overlay, nullptr},
                {"button_prompts", nullptr, &g_button_prompts}},  // game settings kept in the settings file
    .load = LoadOptions,                         // reads more from the settings file's toml::table
    .save = SaveOptions,                  // writes more into it
    .init = InitOptions,                         // after the settings load, before the window and boot
    .loaded = nullptr,                    // image in memory, before its entry point runs
    .window = OffscreenWindow,                    // the game window is up
    .message = OffscreenMessage,                  // window messages first; true takes one
    .ui = OverlayUi,                        // ImGui drawing each frame
    .pad = ControlsPad,                  // keyboard and scripted input on port 0 (input_keys.cpp)
    .rumble = ControlsRumble,                    // changes a rumble before it is set
    .releases = {},                       // one release: the retail xbe
    .view_w = 640,                        // the game draws 640x448 of its 640x480 back buffer (viewport c-96/c-95)
    .view_h = 448,
    .expands = true,                      // src/game/widescreen.cpp widens its projection
    .target_scale = TargetScale,          // the shadow map at shadow_resolution
    .one_release = true,                  // TODO.md item 8: the one supported Xbox release, others refused
    .skip_texture = SkipTexture,          // character_shadows
    .launcher = UnchainedLauncher,        // direct start, or the Configuration app (--configure, first run)
    .release_name = L"Unchained supports one Xbox release of Whiplash: the one whose default.xbe has the SHA-1 below.",
    .texture_edits = PromptsEdits,        // the font's button glyphs for the device in use (prompts.cpp)
    .texture_image = PromptsImage,
};
