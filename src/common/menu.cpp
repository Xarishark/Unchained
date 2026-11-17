// The in-game menu: F1, or Back and Start together on a controller, opens it over the game. The settings that take
// effect at once (display, graphics, HUD and prompts, controls), in the Configuration app's look: dark teal, green
// terminal frames with a tab and a cut corner, the big slanted words. While it is open the game gets no input from the
// controller or the keyboard; the mouse, the keyboard and the controller drive the menu. Closing it (F1, Escape, B, or
// Back and Start again) saves the settings. Settings that need a restart stay in Unchained Configuration.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "app/app.h"
#include "common/fonts.h"
#include "common/version.h"
#include "core/input.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include "app/win32_compat.h"  // <windows.h>, or its messages and key state from SDL on Linux

#include <imgui.h>

extern bool g_perf_overlay, g_invert_camera_x, g_invert_camera_y;
extern uint32_t g_fov_degrees, g_draw_distance, g_camera_speed, g_vibration, g_button_prompts;

namespace {

std::atomic<bool> g_open{false};
std::atomic<bool> g_swallow{false};  // after closing: the game gets nothing until every button and key is let go
bool g_was_open = false;            // the render thread's view, to save on the close
bool g_changed = false;

ImFont* g_body = nullptr;
ImFont* g_head = nullptr;
ImFont* g_term = nullptr;
ImFont* g_word = nullptr;
float g_s = 1;  // the menu's scale: the window's height over 1080

constexpr ImU32 Rgba(int r, int g, int b, int a = 255) { return IM_COL32(r, g, b, a); }
constexpr ImU32 kBack = Rgba(5, 22, 24, 245);
constexpr ImU32 kBack2 = Rgba(9, 38, 40, 245);
constexpr ImU32 kGreen = Rgba(57, 242, 138);
constexpr ImU32 kGreenDim = Rgba(31, 143, 85);
constexpr ImU32 kGreenFaint = Rgba(57, 242, 138, 22);
constexpr ImU32 kRed = Rgba(224, 52, 58);
constexpr ImU32 kWhite = Rgba(235, 248, 240);
constexpr ImU32 kBlack = Rgba(0, 0, 0);

float S(float v) { return v * g_s; }
ImVec4 F(ImU32 c) { return ImGui::ColorConvertU32ToFloat4(c); }

enum Page { kDisplay, kGraphics, kHud, kControls, kPages };
const char* kPageNames[kPages] = {"DISPLAY", "GRAPHICS", "HUD & PROMPTS", "CONTROLS"};
const char* kPageWords[kPages] = {"FREEDOM", "MADNESS", "CRAZY", "FREEDOM"};
int g_page = kDisplay;

// the Configuration app's drawing: a word's vertices leaned right, the big word, the frame with its tab
void Shear(ImDrawList* dl, int start, float y, float k) {
    for (int i = start; i < dl->VtxBuffer.Size; i++) dl->VtxBuffer[i].pos.x += (y - dl->VtxBuffer[i].pos.y) * k;
}

void Word(ImDrawList* dl, float size, ImVec2 at, const char* text, float alpha = 1) {
    ImFont* font = g_word ? g_word : ImGui::GetFont();
    int start = dl->VtxBuffer.Size;
    auto a = [&](ImU32 c) { return (c & 0x00FFFFFF) | (ImU32((c >> 24) * alpha) << 24); };
    float o = std::max(1.0f, size / 40);
    for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 1; dy++)
            if (dx || dy) dl->AddText(font, size, ImVec2(at.x + dx * o, at.y + dy * o), a(kBlack), text);
    dl->AddText(font, size, ImVec2(at.x + o * 1.4f, at.y + o * 1.4f), a(kRed), text);
    dl->AddText(font, size, at, a(kWhite), text);
    Shear(dl, start, at.y + size, 0.22f);
}

void Frame(ImDrawList* dl, ImVec2 a, ImVec2 b, const char* tab) {
    float cut = S(16);
    ImVec2 pts[] = {a, ImVec2(b.x - cut, a.y), ImVec2(b.x, a.y + cut), b, ImVec2(a.x, b.y)};
    dl->AddConvexPolyFilled(pts, 5, kBack2);
    dl->AddPolyline(pts, 5, kGreen, ImDrawFlags_Closed, S(2));
    ImFont* f = g_term ? g_term : ImGui::GetFont();
    float fs = f->FontSize * g_s;
    float tw = f->CalcTextSizeA(fs, 1e9f, 0, tab).x + S(30);
    ImVec2 t[] = {ImVec2(a.x, a.y - S(26)), ImVec2(a.x + tw, a.y - S(26)), ImVec2(a.x + tw + S(12), a.y),
                  ImVec2(a.x, a.y)};
    dl->AddConvexPolyFilled(t, 4, kGreen);
    dl->AddText(f, fs, ImVec2(a.x + S(12), a.y - S(24)), Rgba(5, 22, 24), tab);
}

void Changed() { g_changed = true; }

// a game option's flag by its settings key (GameInfo::options)
bool* OptFlag(const char* key) {
    static bool none = false;
    for (const app::Option& o : app::kGame.options)
        if (std::string(o.key) == key && o.flag) return o.flag;
    return &none;
}

bool Combo(const char* label, uint32_t* value, const std::vector<const char*>& names) {
    int v = int(std::min<uint32_t>(*value, uint32_t(names.size() - 1)));
    if (!ImGui::Combo(label, &v, names.data(), int(names.size()))) return false;
    *value = uint32_t(v);
    Changed();
    return true;
}

// a uint32 setting picked from fixed values
bool Choice(const char* label, uint32_t* value, const std::vector<std::pair<uint32_t, const char*>>& items) {
    int at = 0;
    for (size_t i = 0; i < items.size(); i++)
        if (items[i].first == *value) at = int(i);
    std::vector<const char*> names;
    for (auto& [v, n] : items) names.push_back(n);
    if (!ImGui::Combo(label, &at, names.data(), int(names.size()))) return false;
    *value = items[size_t(at)].first;
    Changed();
    return true;
}

bool Slider(const char* label, uint32_t* value, int lo, int hi, const char* fmt = "%d") {
    int v = int(*value);
    if (!ImGui::SliderInt(label, &v, lo, hi, fmt)) return false;
    *value = uint32_t(v);
    Changed();
    return true;
}

bool SliderS(const char* label, int32_t* value, int lo, int hi) {
    int v = *value;
    if (!ImGui::SliderInt(label, &v, lo, hi)) return false;
    *value = v;
    Changed();
    return true;
}

bool Check(const char* label, bool* value) {
    if (!ImGui::Checkbox(label, value)) return false;
    Changed();
    return true;
}

void Note(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, F(kGreenDim));
    if (g_term) ImGui::PushFont(g_term);
    ImGui::TextWrapped("%s", text);
    if (g_term) ImGui::PopFont();
    ImGui::PopStyleColor();
}

void PageDisplay() {
    app::Settings& s = app::Get();
    uint32_t mode = uint32_t(s.display_mode);
    if (Combo("Window", &mode, {"Fullscreen", "Borderless", "Windowed"})) {
        s.display_mode = app::DisplayMode(mode);
        app::ApplyDisplay();
    }
    uint32_t wide = uint32_t(s.widescreen);
    if (Choice("Widescreen", &wide, {{2, "Wider view (16:9 and ultrawide)"}, {1, "4:3 with bars, as on a TV"},
                                     {0, "Console's widescreen (stretched)"}}))
        s.widescreen = app::Wide(wide);
    Slider("Render resolution", &s.render_percent, 50, 200, "%d%%");
    Note("Of the window's size: below 100% renders smaller and upscales, above it supersamples.");
    Slider("Field of view", &g_fov_degrees, 40, 110, "%d deg");
    uint32_t vsync = uint32_t(s.vsync_mode);
    if (Combo("VSync", &vsync, {"Off", "On", "Adaptive", "Fast", "VRR (G-Sync, FreeSync)"})) {
        s.vsync_mode = app::VSync(vsync);
        s.vsync = s.vsync_mode != app::VSync::Off;
    }
    Choice("Frame rate cap", &s.fps_cap, {{60, "60"}, {1, "Monitor's refresh rate"}, {0, "None"}});
}

void PageGraphics() {
    app::Settings& s = app::Get();
    Combo("Upscaler", &s.upscaler, {"Bilinear", "AMD FSR 1"});
    Slider("Sharpening", &s.sharpen, 0, 100, "%d%%");
    Choice("Texture filtering", &s.aniso, {{0, "As the game asks"}, {2, "2x"}, {4, "4x"}, {8, "8x"}, {16, "16x"}});
    Check("Clean menu and HUD edges", &s.ui_upscale);
    Check("Character shadows", OptFlag("character_shadows"));
    Slider("Draw distance", &g_draw_distance, 100, 1000, "%d%%");
    ImGui::Separator();
    SliderS("Exposure", &s.grade_exposure, -200, 200);
    Slider("Contrast", &s.grade_contrast, 50, 200, "%d%%");
    Slider("Saturation", &s.grade_saturation, 0, 200, "%d%%");
    SliderS("Temperature", &s.grade_temperature, -100, 100);
    SliderS("Tint", &s.grade_tint, -100, 100);
    Combo("Colour vision", &s.colour_filter, {"Off", "Protanopia", "Deuteranopia", "Tritanopia"});
    Slider("Colour vision strength", &s.colour_filter_strength, 0, 200, "%d%%");
}

void PageHud() {
    app::Settings& s = app::Get();
    Slider("HUD at the edges", &s.hud_edge, 0, 100, "%d%%");
    Note("0% keeps the HUD where the console put it, 100% moves it out to the screen's edges.");
    Check("Show the HUD", &s.hud_visible);
    Combo("Button prompts", &g_button_prompts,
          {"The device in use", "Xbox", "PlayStation", "Nintendo", "Keyboard", "Original Xbox (the console's)"});
    Check("Performance overlay (F10)", &g_perf_overlay);
}

void PageControls() {
    Slider("Camera speed", &g_camera_speed, 25, 300, "%d%%");
    Check("Invert camera left-right", &g_invert_camera_x);
    Check("Invert camera up-down", &g_invert_camera_y);
    Slider("Vibration", &g_vibration, 0, 100, "%d%%");
    Note("Keys are changed in Unchained Configuration, or in settings.toml's [keys] table.");
}

void Style() {
    ImGuiStyle& st = ImGui::GetStyle();
    st.FrameRounding = 0;
    st.FramePadding = ImVec2(S(10), S(6));
    st.ItemSpacing = ImVec2(S(12), S(10));
    st.GrabMinSize = S(14);
    st.WindowPadding = ImVec2(0, 0);
    ImVec4* c = st.Colors;
    c[ImGuiCol_Text] = F(kWhite);
    c[ImGuiCol_WindowBg] = F(Rgba(0, 0, 0, 0));
    c[ImGuiCol_ChildBg] = F(Rgba(0, 0, 0, 0));
    c[ImGuiCol_PopupBg] = F(kBack2);
    c[ImGuiCol_Border] = F(kGreenDim);
    c[ImGuiCol_FrameBg] = F(Rgba(6, 30, 32));
    c[ImGuiCol_FrameBgHovered] = F(Rgba(12, 60, 50));
    c[ImGuiCol_FrameBgActive] = F(Rgba(16, 80, 60));
    c[ImGuiCol_Button] = F(Rgba(10, 60, 46));
    c[ImGuiCol_ButtonHovered] = F(kGreenDim);
    c[ImGuiCol_ButtonActive] = F(kGreen);
    c[ImGuiCol_CheckMark] = F(kGreen);
    c[ImGuiCol_SliderGrab] = F(kGreen);
    c[ImGuiCol_SliderGrabActive] = F(kWhite);
    c[ImGuiCol_Header] = F(Rgba(31, 143, 85, 160));
    c[ImGuiCol_HeaderHovered] = F(Rgba(31, 143, 85, 230));
    c[ImGuiCol_HeaderActive] = F(kGreen);
    c[ImGuiCol_Separator] = F(kGreenDim);
    c[ImGuiCol_NavHighlight] = F(kGreen);
}

void Close() {
    g_open = false;
    g_swallow = true;
}

}  // namespace

// GameInfo::window: Windows' own fonts for the menu, before the renderer builds ImGui's font texture
void MenuFonts() {
    if (!ImGui::GetCurrentContext()) return;
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();  // the first font stays ImGui's default: the performance overlay's
    auto load = [&](const wchar_t* file, float size, bool bold = false) -> ImFont* {
        std::filesystem::path p = fonts::File(file, bold);  // Windows' font, or on Linux the nearest (common/fonts.cpp)
        if (p.empty()) return nullptr;
        return io.Fonts->AddFontFromFileTTF(p.string().c_str(), size);
    };
    g_body = load(L"bahnschrift.ttf", 24);
    g_head = load(L"bahnschrift.ttf", 34, true);  // bold only on Linux: the narrow stand-in reads thinner
    g_term = load(L"consola.ttf", 19);
    g_word = load(L"impact.ttf", 96);
}

bool MenuOpen() { return g_open; }

// window thread: F1 opens and closes, Escape closes
bool MenuMessage(uint32_t msg, uintptr_t wp) {
    if (msg != WM_KEYDOWN && msg != WM_SYSKEYDOWN) return false;
    if (wp == VK_F1) {
        if (g_open) Close();
        else g_open = true;
        return true;
    }
    if (wp == VK_ESCAPE && g_open) {
        Close();
        return true;
    }
    return false;
}

// port 0's controller, before the keyboard joins it (controls.cpp): Back and Start together open or close the menu;
// while it is open the controller drives ImGui's navigation. True: the game gets nothing this poll.
bool MenuPad(const input::Pad& p) {
    constexpr uint16_t kStart = 16, kBack = 32;
    constexpr uint16_t kUp = 1, kDown = 2, kLeft = 4, kRight = 8;
    constexpr uint8_t kPressed = 64;
    constexpr int kStickDead = 16000;
    static uint16_t was = 0;
    static bool b_was = false;
    bool combo = (p.buttons & (kStart | kBack)) == (kStart | kBack);
    bool combo_was = (was & (kStart | kBack)) == (kStart | kBack);
    was = p.buttons;
    if (combo && !combo_was) {
        if (g_open) Close();
        else g_open = true;
    }
    if (!g_open) return false;
    std::lock_guard lock(app::UiLock());
    if (!ImGui::GetCurrentContext()) return true;
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    io.AddKeyEvent(ImGuiKey_GamepadDpadUp, (p.buttons & kUp) || p.ly > kStickDead);
    io.AddKeyEvent(ImGuiKey_GamepadDpadDown, (p.buttons & kDown) || p.ly < -kStickDead);
    io.AddKeyEvent(ImGuiKey_GamepadDpadLeft, (p.buttons & kLeft) || p.lx < -kStickDead);
    io.AddKeyEvent(ImGuiKey_GamepadDpadRight, (p.buttons & kRight) || p.lx > kStickDead);
    io.AddKeyEvent(ImGuiKey_GamepadFaceDown, p.analog[0] > kPressed);   // A: press
    io.AddKeyEvent(ImGuiKey_GamepadFaceRight, p.analog[1] > kPressed);  // B: back
    io.AddKeyEvent(ImGuiKey_GamepadL1, p.analog[5] > kPressed);         // White (left shoulder): the page before
    io.AddKeyEvent(ImGuiKey_GamepadR1, p.analog[4] > kPressed);         // Black (right shoulder): the next page
    // B with no list open closes the menu
    bool b = p.analog[1] > kPressed;
    if (b && !b_was && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId)) Close();
    b_was = b;
    return true;
}

// after the menu closed: true while anything is still held, so the buttons that closed it don't reach the game
bool MenuSwallow(const input::Pad& p) {
    if (!g_swallow) return false;
    bool held = p.buttons || p.lx > 8000 || p.lx < -8000 || p.ly > 8000 || p.ly < -8000;
    for (uint8_t a : p.analog) held |= a > 32;
    if (!held) g_swallow = false;
    return held;
}

// each frame inside ImGui's (from OverlayUi)
void MenuUi() {
    bool open = g_open;
    if (g_was_open && !open && g_changed) {
        app::Save();  // the settings as left, for the next start
        g_changed = false;
    }
    if (open && !g_was_open) {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    }
    g_was_open = open;
    if (!open) return;

    ImGuiIO& io = ImGui::GetIO();
    g_s = std::clamp(io.DisplaySize.y / 1080.0f, 0.6f, 3.0f);
    ImGuiStyle saved = ImGui::GetStyle();
    Style();
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false)) g_page = (g_page + kPages - 1) % kPages;
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false)) g_page = (g_page + 1) % kPages;

    // the game dimmed behind, the terminal's scanlines over it
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    bg->AddRectFilled(ImVec2(0, 0), io.DisplaySize, Rgba(0, 0, 0, 150));
    for (float y = 0; y < io.DisplaySize.y; y += S(3)) bg->AddLine(ImVec2(0, y), ImVec2(io.DisplaySize.x, y), Rgba(0, 0, 0, 40), 1);

    float w = std::min(S(1240), io.DisplaySize.x - S(60)), h = std::min(S(760), io.DisplaySize.y - S(60));
    ImVec2 at((io.DisplaySize.x - w) / 2, (io.DisplaySize.y - h) / 2);
    ImGui::SetNextWindowPos(at);
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("##unchained-menu", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetWindowFontScale(g_s);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilledMultiColor(at, ImVec2(at.x + w, at.y + h), kBack2, kBack2, kBack, kBack);
    for (float x = at.x; x < at.x + w; x += S(40)) dl->AddLine(ImVec2(x, at.y), ImVec2(x, at.y + h), kGreenFaint);
    for (float y = at.y; y < at.y + h; y += S(40)) dl->AddLine(ImVec2(at.x, y), ImVec2(at.x + w, y), kGreenFaint);
    dl->AddRect(at, ImVec2(at.x + w, at.y + h), kGreenDim, 0, 0, S(1));

    // header: the name, the caption, the version
    Word(dl, S(54), ImVec2(at.x + S(28), at.y + S(14)), "UNCHAINED");
    ImFont* term = g_term ? g_term : ImGui::GetFont();
    float ts = term->FontSize * g_s;
    dl->AddText(term, ts, ImVec2(at.x + S(34), at.y + S(80)), kGreen, "WHIPLASH  //  PC PORT  //  IN-GAME MENU");
    std::string build = std::string("BUILD ") + kUnchainedVersion;
    float bw = term->CalcTextSizeA(ts, 1e9f, 0, build.c_str()).x;
    dl->AddText(term, ts, ImVec2(at.x + w - bw - S(28), at.y + S(24)), kGreenDim, build.c_str());
    dl->AddLine(ImVec2(at.x + S(28), at.y + S(108)), ImVec2(at.x + w - S(28), at.y + S(108)), kGreenDim, S(1));

    // the pages, as a terminal's list
    float top = at.y + S(132), left = at.x + S(28), nav = S(230);
    ImGui::SetCursorScreenPos(ImVec2(left, top));
    ImGui::BeginGroup();
    if (g_term) ImGui::PushFont(g_term);
    for (int i = 0; i < kPages; i++) {
        bool on = g_page == i;
        ImGui::PushStyleColor(ImGuiCol_Text, F(on ? Rgba(5, 22, 24) : kGreen));
        ImGui::PushStyleColor(ImGuiCol_Header, F(kGreen));
        std::string label = std::string(on ? "> " : "  ") + kPageNames[i];
        if (ImGui::Selectable(label.c_str(), on, 0, ImVec2(nav, S(32)))) g_page = i;
        ImGui::PopStyleColor(2);
    }
    if (g_term) ImGui::PopFont();
    ImGui::EndGroup();

    // the page in its frame, its word faint behind
    ImVec2 a(left + nav + S(28), top + S(10)), b(at.x + w - S(28), at.y + h - S(70));
    Frame(dl, a, b, kPageNames[g_page]);
    float wsize = S(130);
    ImFont* wf = g_word ? g_word : ImGui::GetFont();
    float ww = wf->CalcTextSizeA(wsize, 1e9f, 0, kPageWords[g_page]).x;
    Word(dl, wsize, ImVec2(b.x - ww - S(50), b.y - wsize - S(8)), kPageWords[g_page], 0.10f);
    ImGui::SetCursorScreenPos(ImVec2(a.x + S(22), a.y + S(22)));
    ImGui::BeginChild("page", ImVec2(b.x - a.x - S(44), b.y - a.y - S(36)), false);
    ImGui::SetWindowFontScale(g_s);
    if (g_body) ImGui::PushFont(g_body);
    ImGui::PushItemWidth(S(330));
    switch (g_page) {
        case kDisplay: PageDisplay(); break;
        case kGraphics: PageGraphics(); break;
        case kHud: PageHud(); break;
        case kControls: PageControls(); break;
        default: break;
    }
    ImGui::PopItemWidth();
    if (g_body) ImGui::PopFont();
    ImGui::EndChild();

    // how to leave
    dl->AddText(term, ts, ImVec2(left, at.y + h - S(44)), kGreenDim,
                "F1, ESC, B OR BACK + START: BACK TO THE GAME   //   LB / RB: PAGES   //   SAVED WHEN CLOSED");
    ImGui::End();
    ImGui::GetStyle() = saved;
}
