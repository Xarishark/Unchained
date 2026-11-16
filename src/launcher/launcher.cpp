// Unchained's Configuration app and first-run setup (TODO.md items 2, 44, L1, L2): a window of its own, drawn with
// Dear ImGui on D3D12, before the game is looked for (GameInfo::launcher). Unchained.exe starts the game directly when
// its files are there; "Unchained.exe --configure", or a first run without them, opens this instead.
//
// The look follows Whiplash's Genron Corporation lab: dark teal, green terminal frames with a tab and cut corners,
// scanlines, and the game's big slanted words (FREEDOM, CRAZY, MADNESS) for headings. Nothing of the game's art is in
// the repository: the shapes are drawn here and the fonts are Windows' own (Bahnschrift, Consolas).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <shobjidl.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <string>
#include <vector>

#include <imgui.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>

#include "app/app.h"
#include "launcher/setup.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// game_info.cpp: the graphics preset, which the app sets and applies
std::string& GraphicsPreset();
void ApplyGraphicsPreset();

namespace launcher {

using Microsoft::WRL::ComPtr;
namespace fs = std::filesystem;

namespace {

// ---- colours: the lab's teal and the terminal's green

constexpr ImU32 Rgba(int r, int g, int b, int a = 255) { return IM_COL32(r, g, b, a); }
constexpr ImU32 kBack = Rgba(5, 22, 24);
constexpr ImU32 kBack2 = Rgba(9, 38, 40);
constexpr ImU32 kGreen = Rgba(57, 242, 138);
constexpr ImU32 kGreenDim = Rgba(31, 143, 85);
constexpr ImU32 kGreenFaint = Rgba(57, 242, 138, 22);
constexpr ImU32 kPanel = Rgba(6, 30, 32, 225);
constexpr ImU32 kRed = Rgba(224, 52, 58);
constexpr ImU32 kWhite = Rgba(235, 248, 240);
constexpr ImU32 kBlack = Rgba(0, 0, 0);

// ---- D3D12: just enough to draw ImGui

constexpr UINT kBuffers = 2;
struct Gfx {
    ComPtr<ID3D12Device> dev;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain3> swap;
    ComPtr<ID3D12DescriptorHeap> rtv, srv;
    ComPtr<ID3D12CommandAllocator> alloc[kBuffers];
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Resource> back[kBuffers];
    ComPtr<ID3D12Fence> fence;
    UINT64 next = 0, done[kBuffers] = {};
    HANDLE event = nullptr;
    UINT rtv_inc = 0;
    bool tearing = false;
} g;

HWND g_hwnd = nullptr;
float g_dpi = 1;
bool g_quit = false, g_play = false;

void WaitFor(UINT64 value) {
    if (g.fence->GetCompletedValue() < value) {
        g.fence->SetEventOnCompletion(value, g.event);
        WaitForSingleObject(g.event, INFINITE);
    }
}

void Idle() {
    g.queue->Signal(g.fence.Get(), ++g.next);
    WaitFor(g.next);
}

void MakeTargets() {
    for (UINT i = 0; i < kBuffers; i++) {
        g.swap->GetBuffer(i, IID_PPV_ARGS(&g.back[i]));
        D3D12_CPU_DESCRIPTOR_HANDLE h = g.rtv->GetCPUDescriptorHandleForHeapStart();
        h.ptr += SIZE_T(i) * g.rtv_inc;
        g.dev->CreateRenderTargetView(g.back[i].Get(), nullptr, h);
    }
}

bool InitGfx(HWND hwnd) {
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g.dev)))) return false;
    D3D12_COMMAND_QUEUE_DESC qd{};
    g.dev->CreateCommandQueue(&qd, IID_PPV_ARGS(&g.queue));
    D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kBuffers};
    g.dev->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&g.rtv));
    g.rtv_inc = g.dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_DESCRIPTOR_HEAP_DESC sd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE};
    g.dev->CreateDescriptorHeap(&sd, IID_PPV_ARGS(&g.srv));
    for (auto& a : g.alloc) g.dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&a));
    g.dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g.alloc[0].Get(), nullptr, IID_PPV_ARGS(&g.list));
    g.list->Close();
    g.dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g.fence));
    g.event = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return false;
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.BufferCount = kBuffers;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.SampleDesc.Count = 1;
    ComPtr<IDXGISwapChain1> sc1;
    if (FAILED(factory->CreateSwapChainForHwnd(g.queue.Get(), hwnd, &desc, nullptr, nullptr, &sc1))) return false;
    sc1.As(&g.swap);
    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    MakeTargets();
    return true;
}

void Resize() {
    if (!g.swap) return;
    Idle();
    for (auto& b : g.back) b.Reset();
    g.swap->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    MakeTargets();
}

void Present() {
    UINT i = g.swap->GetCurrentBackBufferIndex();
    WaitFor(g.done[i]);
    g.alloc[i]->Reset();
    g.list->Reset(g.alloc[i].Get(), nullptr);
    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = g.back[i].Get();
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    b.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    g.list->ResourceBarrier(1, &b);
    D3D12_CPU_DESCRIPTOR_HANDLE rt = g.rtv->GetCPUDescriptorHandleForHeapStart();
    rt.ptr += SIZE_T(i) * g.rtv_inc;
    const float clear[4] = {5 / 255.0f, 22 / 255.0f, 24 / 255.0f, 1};
    g.list->ClearRenderTargetView(rt, clear, 0, nullptr);
    g.list->OMSetRenderTargets(1, &rt, FALSE, nullptr);
    ID3D12DescriptorHeap* heaps[] = {g.srv.Get()};
    g.list->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g.list.Get());
    std::swap(b.Transition.StateBefore, b.Transition.StateAfter);
    g.list->ResourceBarrier(1, &b);
    g.list->Close();
    ID3D12CommandList* lists[] = {g.list.Get()};
    g.queue->ExecuteCommandLists(1, lists);
    g.swap->Present(1, 0);
    g.queue->Signal(g.fence.Get(), ++g.next);
    g.done[i] = g.next;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) return 1;
    switch (msg) {
        case WM_SIZE:
            if (wp != SIZE_MINIMIZED) Resize();
            return 0;
        case WM_GETMINMAXINFO: {
            auto* mm = reinterpret_cast<MINMAXINFO*>(lp);
            mm->ptMinTrackSize = {LONG(900 * g_dpi), LONG(600 * g_dpi)};
            return 0;
        }
        case WM_CLOSE:
            g_quit = true;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ---- fonts: Windows' own

ImFont* g_body = nullptr;
ImFont* g_term = nullptr;
ImFont* g_head = nullptr;
ImFont* g_word = nullptr;

ImFont* LoadFont(const wchar_t* file, float size) {
    wchar_t dir[MAX_PATH];
    GetWindowsDirectoryW(dir, MAX_PATH);
    fs::path path = fs::path(dir) / L"Fonts" / file;
    std::error_code ec;
    if (!fs::exists(path, ec)) return nullptr;
    return ImGui::GetIO().Fonts->AddFontFromFileTTF(path.string().c_str(), size * g_dpi);
}

void LoadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    g_body = LoadFont(L"bahnschrift.ttf", 19);
    if (!g_body) g_body = io.Fonts->AddFontDefault();
    g_term = LoadFont(L"consola.ttf", 16);
    if (!g_term) g_term = g_body;
    g_head = LoadFont(L"bahnschrift.ttf", 30);
    if (!g_head) g_head = g_body;
    g_word = LoadFont(L"impact.ttf", 120);
    if (!g_word) g_word = g_head;
    io.FontDefault = g_body;
}

// ---- drawing: the lab's frames and words

float S(float v) { return v * g_dpi; }

// slants what was drawn into dl since vtx_start, about the line y (the game's words lean right)
void Shear(ImDrawList* dl, int vtx_start, float y, float k) {
    for (int i = vtx_start; i < dl->VtxBuffer.Size; i++) dl->VtxBuffer[i].pos.x += (y - dl->VtxBuffer[i].pos.y) * k;
}

// a big slanted word: white, a red edge offset down-right, a black outline (FREEDOM, CRAZY, MADNESS)
void Word(ImDrawList* dl, ImFont* font, float size, ImVec2 at, const char* text, float alpha = 1) {
    int start = dl->VtxBuffer.Size;
    auto a = [&](ImU32 c) { return (c & 0x00FFFFFF) | (ImU32((c >> 24) * alpha) << 24); };
    float o = S(3);
    for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 1; dy++)
            if (dx || dy) dl->AddText(font, size, ImVec2(at.x + dx * o, at.y + dy * o), a(kBlack), text);
    dl->AddText(font, size, ImVec2(at.x + S(4), at.y + S(4)), a(kRed), text);
    dl->AddText(font, size, at, a(kWhite), text);
    Shear(dl, start, at.y + size, 0.22f);
}

// the game's menu frame: a tab with a slanted right edge on top, the box's top right corner cut off
void Frame(ImDrawList* dl, ImVec2 a, ImVec2 b, const char* tab, ImFont* font) {
    float cut = S(18), th = S(26), tw = font->CalcTextSizeA(font->FontSize, 1e9f, 0, tab).x + S(40);
    ImVec2 box[] = {{a.x, a.y + th}, {b.x - cut, a.y + th}, {b.x, a.y + th + cut}, {b.x, b.y}, {a.x, b.y}};
    dl->AddConvexPolyFilled(box, 5, kPanel);
    dl->AddPolyline(box, 5, kGreen, ImDrawFlags_Closed, S(2));
    ImVec2 t[] = {{a.x + S(10), a.y + th}, {a.x + S(10), a.y}, {a.x + S(10) + tw - S(14), a.y},
                  {a.x + S(10) + tw, a.y + S(14)}, {a.x + S(10) + tw, a.y + th}};
    dl->AddConvexPolyFilled(t, 5, Rgba(57, 242, 138, 60));
    dl->AddPolyline(t, 5, kGreen, 0, S(2));
    dl->AddText(font, font->FontSize, ImVec2(a.x + S(22), a.y + S(2)), kGreen, tab);
}

// the room: teal, a faint grid and the terminal's scanlines
void Background(ImDrawList* dl, ImVec2 size) {
    dl->AddRectFilledMultiColor(ImVec2(0, 0), size, kBack2, kBack2, kBack, kBack);
    float step = S(40);
    for (float x = 0; x < size.x; x += step) dl->AddLine(ImVec2(x, 0), ImVec2(x, size.y), kGreenFaint, 1);
    for (float y = 0; y < size.y; y += step) dl->AddLine(ImVec2(0, y), ImVec2(size.x, y), kGreenFaint, 1);
    for (float y = 0; y < size.y; y += S(3)) dl->AddLine(ImVec2(0, y), ImVec2(size.x, y), Rgba(0, 0, 0, 40), 1);
}

// ---- settings: the SDK's and the game's (GameInfo::options)

uint32_t* OptValue(const char* key) {
    for (const app::Option& o : app::kGame.options)
        if (!std::strcmp(o.key, key)) return o.value;
    static uint32_t none = 0;
    return &none;
}
bool* OptFlag(const char* key) {
    for (const app::Option& o : app::kGame.options)
        if (!std::strcmp(o.key, key)) return o.flag;
    static bool none = false;
    return &none;
}

bool g_dirty = false;

// a changed graphics setting leaves the preset: the player's own choice stays as set
void Touched(bool graphics) {
    g_dirty = true;
    if (graphics) GraphicsPreset() = "custom";
}

bool Combo(const char* label, uint32_t* value, const std::vector<const char*>& names, bool graphics = false) {
    int v = int(std::min<uint32_t>(*value, uint32_t(names.size() - 1)));
    bool changed = ImGui::Combo(label, &v, names.data(), int(names.size()));
    if (changed) {
        *value = uint32_t(v);
        Touched(graphics);
    }
    return changed;
}

bool Slider(const char* label, uint32_t* value, int lo, int hi, const char* fmt = "%d", bool graphics = false) {
    int v = int(*value);
    bool changed = ImGui::SliderInt(label, &v, lo, hi, fmt);
    if (changed) {
        *value = uint32_t(v);
        Touched(graphics);
    }
    return changed;
}

bool SliderS(const char* label, int32_t* value, int lo, int hi, const char* fmt = "%d") {
    int v = *value;
    bool changed = ImGui::SliderInt(label, &v, lo, hi, fmt);
    if (changed) {
        *value = v;
        Touched(true);
    }
    return changed;
}

bool Check(const char* label, bool* value, bool graphics = false) {
    bool changed = ImGui::Checkbox(label, value);
    if (changed) Touched(graphics);
    return changed;
}

// a uint32 setting picked from fixed values
bool Choice(const char* label, uint32_t* value, const std::vector<std::pair<uint32_t, const char*>>& items,
            bool graphics = false) {
    int at = 0;
    for (size_t i = 0; i < items.size(); i++)
        if (items[i].first == *value) at = int(i);
    std::vector<const char*> names;
    for (auto& [v, n] : items) names.push_back(n);
    if (!ImGui::Combo(label, &at, names.data(), int(names.size()))) return false;
    *value = items[size_t(at)].first;
    Touched(graphics);
    return true;
}

void Note(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(kGreenDim));
    ImGui::PushFont(g_term);
    ImGui::TextWrapped("%s", text);
    ImGui::PopFont();
    ImGui::PopStyleColor();
}

// ---- pages

enum Page { kPlay, kDisplay, kGraphics, kControls, kSound, kFiles, kAbout, kPages };
const char* kPageNames[kPages] = {"PLAY", "DISPLAY", "GRAPHICS", "CONTROLS", "SOUND & SAVES", "GAME FILES", "ABOUT"};
const char* kPageWords[kPages] = {"FREEDOM", "MADNESS", "CRAZY", "FREEDOM", "CRAZY", "MADNESS", "FREEDOM"};
Page g_page = kPlay;

std::wstring PickFile(bool folder) {
    std::wstring out;
    ComPtr<IFileOpenDialog> dlg;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg)))) return out;
    DWORD opts = 0;
    dlg->GetOptions(&opts);
    dlg->SetOptions(opts | FOS_FORCEFILESYSTEM | (folder ? FOS_PICKFOLDERS : 0));
    if (!folder) {
        COMDLG_FILTERSPEC spec[] = {{L"Xbox disc image", L"*.iso"}, {L"All files", L"*.*"}};
        dlg->SetFileTypes(2, spec);
        dlg->SetTitle(L"Your Whiplash disc image");
    } else {
        dlg->SetTitle(L"The folder with Whiplash's extracted files (default.xbe and the rest)");
    }
    if (FAILED(dlg->Show(g_hwnd))) return out;
    ComPtr<IShellItem> item;
    if (FAILED(dlg->GetResult(&item))) return out;
    PWSTR path = nullptr;
    if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
        out = path;
        CoTaskMemFree(path);
    }
    return out;
}

void PagePlay() {
    bool ready = GameFilesPresent();
    ImGui::PushFont(g_head);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kGreen), ready ? "SUBJECTS READY" : "GAME FILES MISSING");
    ImGui::PopFont();
    ImGui::Spacing();
    if (ready) {
        Note("Spanx and Redmond are chained up in Genron's lab. Press PLAY to break them out.");
    } else {
        Note("Unchained needs your own copy of Whiplash for the original Xbox: its disc image (.iso) or extracted "
             "files. Open GAME FILES to set it up once.");
    }
    ImGui::Spacing();
    ImGui::Spacing();
    std::vector<const char*> presets = {"custom", "original", "low", "high", "max"};
    std::string& p = GraphicsPreset();
    int at = 0;
    for (size_t i = 0; i < presets.size(); i++)
        if (p == presets[i]) at = int(i);
    ImGui::SetNextItemWidth(S(260));
    if (ImGui::Combo("Graphics preset", &at, presets.data(), int(presets.size()))) {
        p = presets[size_t(at)];
        ApplyGraphicsPreset();
        g_dirty = true;
    }
    Note("original: the console's picture, sharp at your resolution. low: 75% scale with FSR. high: native, 16x "
         "filtering, 2048 shadows. max: 200% supersampling, 4096 shadows. custom: as set on the other pages.");
    ImGui::Spacing();
    app::Settings& s = app::Get();
    uint32_t mode = uint32_t(s.display_mode);
    ImGui::SetNextItemWidth(S(260));
    if (Combo("Window", &mode, {"Fullscreen", "Borderless", "Windowed"})) s.display_mode = app::DisplayMode(mode);
}

std::vector<std::pair<uint32_t, uint32_t>> Resolutions() {
    std::vector<std::pair<uint32_t, uint32_t>> out;
    DEVMODEW dm{};
    dm.dmSize = sizeof(dm);
    for (DWORD i = 0; EnumDisplaySettingsW(nullptr, i, &dm); i++) {
        std::pair<uint32_t, uint32_t> r{dm.dmPelsWidth, dm.dmPelsHeight};
        if (r.first >= 640 && std::find(out.begin(), out.end(), r) == out.end()) out.push_back(r);
    }
    std::sort(out.begin(), out.end());
    return out;
}

void PageDisplay() {
    app::Settings& s = app::Get();
    float w = S(300);
    ImGui::PushItemWidth(w);
    uint32_t mode = uint32_t(s.display_mode);
    if (Combo("Window", &mode, {"Fullscreen", "Borderless", "Windowed"})) s.display_mode = app::DisplayMode(mode);
    static std::vector<std::pair<uint32_t, uint32_t>> res = Resolutions();
    std::vector<std::string> names = {"Desktop"};
    int at = 0;
    for (size_t i = 0; i < res.size(); i++) {
        names.push_back(std::to_string(res[i].first) + " x " + std::to_string(res[i].second));
        if (res[i].first == s.width && res[i].second == s.height) at = int(i + 1);
    }
    std::vector<const char*> cnames;
    for (auto& n : names) cnames.push_back(n.c_str());
    if (ImGui::Combo("Resolution", &at, cnames.data(), int(cnames.size()))) {
        s.width = at ? res[size_t(at - 1)].first : 0;
        s.height = at ? res[size_t(at - 1)].second : 0;
        Touched(false);
    }
    uint32_t wide = uint32_t(s.widescreen);
    // the settings' order: console, off, expand
    std::vector<std::pair<uint32_t, const char*>> wides = {{2, "Wider view (16:9 and ultrawide)"},
                                                           {1, "4:3 with bars, as on a TV"},
                                                           {0, "Console's widescreen (stretched)"}};
    if (Choice("Widescreen", &wide, wides)) s.widescreen = app::Wide(wide);
    Slider("HUD at the edges", &s.hud_edge, 0, 100, "%d%%");
    Note("0% keeps the HUD where the console put it, 100% moves it out to the screen's edges.");
    Slider("Field of view", OptValue("fov"), 40, 110, "%d deg");
    uint32_t vsync = uint32_t(s.vsync_mode);
    if (Combo("VSync", &vsync, {"Off", "On", "Adaptive", "Fast", "VRR (G-Sync, FreeSync)"})) {
        s.vsync_mode = app::VSync(vsync);
        s.vsync = s.vsync_mode != app::VSync::Off;
    }
    Choice("Frame rate cap", &s.fps_cap, {{60, "60"}, {1, "Monitor's refresh rate"}, {0, "None"}});
    Note("Above 60 the game is checked in the intro and menus only so far.");
    ImGui::PopItemWidth();
}

void PageGraphics() {
    app::Settings& s = app::Get();
    ImGui::PushItemWidth(S(300));
    Slider("Render scale", &s.render_percent, 50, 200, "%d%%", true);
    Combo("Upscaler", &s.upscaler, {"Bilinear", "AMD FSR 1"}, true);
    Slider("Sharpening", &s.sharpen, 0, 100, "%d%%", true);
    Choice("Texture filtering", &s.aniso, {{0, "As the game asks"}, {2, "2x"}, {4, "4x"}, {8, "8x"}, {16, "16x"}}, true);
    Choice("Shadow detail", OptValue("shadow_resolution"),
           {{0, "With the render scale"}, {1024, "1024"}, {2048, "2048"}, {4096, "4096"}}, true);
    Check("Character shadows", OptFlag("character_shadows"), true);
    Slider("Draw distance", OptValue("draw_distance"), 100, 1000, "%d%%", true);
    ImGui::Separator();
    SliderS("Exposure", &s.grade_exposure, -300, 300, "%d / 100 stop");
    Slider("Contrast", &s.grade_contrast, 50, 200, "%d%%", true);
    Slider("Saturation", &s.grade_saturation, 0, 200, "%d%%", true);
    SliderS("Temperature", &s.grade_temperature, -100, 100);
    SliderS("Tint", &s.grade_tint, -100, 100);
    ImGui::Separator();
    Combo("Colour vision", &s.colour_filter, {"Off", "Protanopia", "Deuteranopia", "Tritanopia"});
    Slider("Colour vision strength", &s.colour_filter_strength, 0, 200, "%d%%");
    Check("Show the HUD", &s.hud_visible);
    ImGui::PopItemWidth();
}

void PageControls() {
    ImGui::PushItemWidth(S(300));
    Check("Mouse turns the camera", OptFlag("mouse_camera"));
    Slider("Mouse sensitivity", OptValue("mouse_sensitivity"), 10, 300, "%d%%");
    Slider("Camera speed", OptValue("camera_speed"), 25, 300, "%d%%");
    Check("Invert camera left-right", OptFlag("invert_camera_x"));
    Check("Invert camera up-down", OptFlag("invert_camera_y"));
    Slider("Vibration", OptValue("vibration"), 0, 100, "%d%%");
    ImGui::PopItemWidth();
    ImGui::Spacing();
    Note("Keys: W A S D move, mouse or I J K L camera, Space jump, left mouse or F attack, right mouse or Left Shift "
         "B, R Y, Q / E triggers, C / V black and white, Enter Start, Esc Back. Every key can be changed in "
         "settings.toml's [keys] table (docs/SETTINGS.md). Any controller works: Xbox, PlayStation, Switch and "
         "others, by button position.");
}

void PageSound() {
    app::Settings& s = app::Get();
    ImGui::PushItemWidth(S(300));
    Combo("Sound output", &s.audio_output, {"Automatic", "Stereo", "Surround", "Mono"});
    ImGui::Separator();
    Slider("Save backups kept", &s.save_backups, 0, 20);
    Note("Each start copies your saves to \"save backups\" beside the game; the newest are kept.");
    static char profile[64] = {};
    static bool loaded = false;
    if (!loaded) {
        std::strncpy(profile, s.save_profile.c_str(), sizeof(profile) - 1);
        loaded = true;
    }
    if (ImGui::InputText("Save profile", profile, sizeof(profile))) {
        s.save_profile = profile;
        Touched(false);
    }
    Note("A name keeps its own set of saves; empty is the main one.");
    ImGui::PopItemWidth();
}

void PageFiles() {
    Setup& st = State();
    Stage stage;
    std::string text, sha;
    float progress;
    {
        std::lock_guard<std::mutex> lock(st.lock);
        stage = st.stage;
        text = st.text;
        progress = st.progress;
        sha = st.found_sha1;
    }
    bool ready = GameFilesPresent();
    ImGui::PushFont(g_head);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(ready ? kGreen : kRed), ready ? "FILES IN PLACE" : "NO GAME FILES");
    ImGui::PopFont();
    Note("Unchained runs the original Xbox Whiplash (USA) from your own copy. Pick its disc image (.iso) or the "
         "folder with its extracted files: default.xbe is checked, then the files are copied beside Unchained. "
         "Nothing of the game comes with Unchained.");
    ImGui::Spacing();
    bool busy = stage == Stage::Checking || stage == Stage::Copying;
    ImGui::BeginDisabled(busy);
    if (ImGui::Button("Choose disc image (.iso)", ImVec2(S(260), S(40)))) {
        std::wstring p = PickFile(false);
        if (!p.empty()) Start(p);
    }
    ImGui::SameLine();
    if (ImGui::Button("Choose extracted folder", ImVec2(S(260), S(40)))) {
        std::wstring p = PickFile(true);
        if (!p.empty()) Start(p);
    }
    ImGui::EndDisabled();
    ImGui::Spacing();
    if (stage != Stage::Idle) {
        ImGui::PushFont(g_term);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::ColorConvertU32ToFloat4(kGreen));
        if (busy) ImGui::ProgressBar(progress, ImVec2(S(530), S(22)));
        ImGui::PopStyleColor();
        ImU32 c = stage == Stage::Failed ? kRed : kGreen;
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(c));
        ImGui::TextWrapped("> %s", text.c_str());
        ImGui::PopStyleColor();
        if (!sha.empty()) ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kGreenDim), "  default.xbe SHA-1 %s", sha.c_str());
        ImGui::PopFont();
        if (busy && ImGui::Button("Stop", ImVec2(S(120), S(32)))) st.cancel = true;
    }
}

void PageAbout() {
    ImGui::PushFont(g_head);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kGreen), "UNCHAINED");
    ImGui::PopFont();
    Note("An unofficial PC port of Whiplash (Crystal Dynamics / Eidos, 2003) for the original Xbox, made by static "
         "recompilation with Xbox XBE Recomp. It contains no game files or code: you need your own copy.");
    ImGui::Spacing();
    Note("Built on: Xbox XBE Recomp, XenonRecomp's function layout, Zydis, toml++, Dear ImGui, SDL, AMD FidelityFX "
         "Super Resolution 1 (MIT).");
    ImGui::Spacing();
    Note("Settings are in settings.toml beside Unchained.exe; docs/SETTINGS.md lists them. Logs are in logs\\, saves in "
         "hdd\\ (backed up to \"save backups\\\").");
}

void Style() {
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = st.FrameRounding = st.GrabRounding = st.ScrollbarRounding = 0;
    st.FrameBorderSize = 1;
    st.FramePadding = ImVec2(S(10), S(6));
    st.ItemSpacing = ImVec2(S(12), S(10));
    st.GrabMinSize = S(14);
    ImVec4* c = st.Colors;
    auto f = [](ImU32 u) { return ImGui::ColorConvertU32ToFloat4(u); };
    c[ImGuiCol_Text] = f(kWhite);
    c[ImGuiCol_TextDisabled] = f(kGreenDim);
    c[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = f(Rgba(6, 30, 32, 250));
    c[ImGuiCol_Border] = f(kGreenDim);
    c[ImGuiCol_FrameBg] = f(Rgba(10, 50, 48, 220));
    c[ImGuiCol_FrameBgHovered] = f(Rgba(20, 80, 70, 230));
    c[ImGuiCol_FrameBgActive] = f(Rgba(30, 110, 85, 240));
    c[ImGuiCol_CheckMark] = f(kGreen);
    c[ImGuiCol_SliderGrab] = f(kGreen);
    c[ImGuiCol_SliderGrabActive] = f(kWhite);
    c[ImGuiCol_Button] = f(Rgba(12, 60, 50, 230));
    c[ImGuiCol_ButtonHovered] = f(Rgba(31, 143, 85, 230));
    c[ImGuiCol_ButtonActive] = f(kGreen);
    c[ImGuiCol_Header] = f(Rgba(31, 143, 85, 200));
    c[ImGuiCol_HeaderHovered] = f(Rgba(31, 143, 85, 230));
    c[ImGuiCol_HeaderActive] = f(kGreen);
    c[ImGuiCol_Separator] = f(kGreenDim);
    c[ImGuiCol_PlotHistogram] = f(kGreen);
}

void DrawUi() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##unchained", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    Background(bg, io.DisplaySize);

    // header: the name as the game writes its words, the lab's caption
    Word(bg, g_head, S(64), ImVec2(S(36), S(14)), "UNCHAINED");
    bg->AddText(g_term, g_term->FontSize, ImVec2(S(44), S(86)), kGreen, "WHIPLASH  //  PC PORT  //  CONFIGURATION");
    const char* corp = "GENRON CORPORATION  -  SUBJECT RELEASE TERMINAL";
    float cw = g_term->CalcTextSizeA(g_term->FontSize, 1e9f, 0, corp).x;
    bg->AddText(g_term, g_term->FontSize, ImVec2(io.DisplaySize.x - cw - S(36), S(30)), kGreenDim, corp);
    bg->AddLine(ImVec2(S(36), S(114)), ImVec2(io.DisplaySize.x - S(36), S(114)), kGreenDim, S(1));

    // the pages, as a terminal's list
    float top = S(140), left = S(36), nav = S(230);
    ImGui::SetCursorPos(ImVec2(left, top));
    ImGui::BeginGroup();
    ImGui::PushFont(g_term);
    for (int i = 0; i < kPages; i++) {
        bool on = g_page == i;
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(on ? kBack : kGreen));
        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::ColorConvertU32ToFloat4(kGreen));
        std::string label = std::string(on ? "> " : "  ") + kPageNames[i];
        if (ImGui::Selectable(label.c_str(), on, 0, ImVec2(nav, S(30)))) g_page = Page(i);
        ImGui::PopStyleColor(2);
    }
    ImGui::PopFont();
    ImGui::EndGroup();

    // the page in the game's frame, its word behind it
    ImVec2 a(left + nav + S(30), top), b(io.DisplaySize.x - S(36), io.DisplaySize.y - S(110));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    Frame(dl, a, b, kPageNames[g_page], g_term);
    float wsize = S(150);
    float ww = g_word->CalcTextSizeA(wsize, 1e9f, 0, kPageWords[g_page]).x;
    Word(dl, g_word, wsize, ImVec2(b.x - ww - S(60), b.y - wsize - S(10)), kPageWords[g_page], 0.12f);
    ImGui::SetCursorScreenPos(ImVec2(a.x + S(24), a.y + S(46)));
    ImGui::BeginChild("page", ImVec2(b.x - a.x - S(48), b.y - a.y - S(64)), false);
    switch (g_page) {
        case kPlay: PagePlay(); break;
        case kDisplay: PageDisplay(); break;
        case kGraphics: PageGraphics(); break;
        case kControls: PageControls(); break;
        case kSound: PageSound(); break;
        case kFiles: PageFiles(); break;
        case kAbout: PageAbout(); break;
        default: break;
    }
    ImGui::EndChild();

    // play and quit, bottom right; play waits for the game's files
    bool ready = GameFilesPresent();
    ImGui::SetCursorScreenPos(ImVec2(io.DisplaySize.x - S(36) - S(470), io.DisplaySize.y - S(84)));
    ImGui::PushFont(g_head);
    if (ImGui::Button("QUIT", ImVec2(S(170), S(56)))) g_quit = true;
    ImGui::SameLine(0, S(20));
    ImGui::BeginDisabled(!ready);
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(Rgba(150, 30, 36)));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(kRed));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorConvertU32ToFloat4(Rgba(255, 90, 90)));
    if (ImGui::Button(ready ? "PLAY" : "SET UP FIRST", ImVec2(S(280), S(56)))) g_play = true;
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();
    ImGui::PopFont();
    if (!ready && g_page != kFiles) {
        ImGui::SetCursorScreenPos(ImVec2(S(36), io.DisplaySize.y - S(70)));
        ImGui::PushFont(g_term);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kRed), "> GAME FILES are needed before the first run.");
        ImGui::PopFont();
    }
    ImGui::End();
}

}  // namespace

bool Run(bool first_run) {
    HRESULT co = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    HINSTANCE inst = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{sizeof(wc)};
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    wc.lpszClassName = L"UnchainedConfiguration";
    RegisterClassExW(&wc);
    g_dpi = GetDpiForSystem() / 96.0f;
    int w = int(1240 * g_dpi), h = int(780 * g_dpi);
    RECT work;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    w = std::min(w, int(work.right - work.left));
    h = std::min(h, int(work.bottom - work.top));
    g_hwnd = CreateWindowExW(0, wc.lpszClassName, L"Unchained Configuration", WS_OVERLAPPEDWINDOW,
                             work.left + (work.right - work.left - w) / 2, work.top + (work.bottom - work.top - h) / 2, w,
                             h, nullptr, nullptr, inst, nullptr);
    bool ok = g_hwnd && InitGfx(g_hwnd);
    if (ok) {
        ShowWindow(g_hwnd, SW_SHOW);
        UpdateWindow(g_hwnd);
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
        LoadFonts();
        Style();
        ImGui_ImplWin32_Init(g_hwnd);
        ImGui_ImplDX12_Init(g.dev.Get(), int(kBuffers), DXGI_FORMAT_R8G8B8A8_UNORM, g.srv.Get(),
                            g.srv->GetCPUDescriptorHandleForHeapStart(), g.srv->GetGPUDescriptorHandleForHeapStart());
        if (first_run) g_page = kFiles;
        // UNCHAINED_SETUP_SOURCE=<image or folder>: the setup starts on it (tests of the first run)
        if (const wchar_t* src = _wgetenv(L"UNCHAINED_SETUP_SOURCE")) Start(src);
        // UNCHAINED_CONFIG_PAGE=<n>: opens on that page (screenshots of each page in tests)
        if (const char* pg = std::getenv("UNCHAINED_CONFIG_PAGE")) g_page = Page(std::clamp(std::atoi(pg), 0, kPages - 1));
        while (!g_quit && !g_play) {
            MSG msg;
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            if (IsIconic(g_hwnd)) {
                Sleep(50);
                continue;
            }
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            DrawUi();
            ImGui::Render();
            Present();
        }
        Idle();
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
    g = Gfx{};
    if (g_hwnd) DestroyWindow(g_hwnd);
    UnregisterClassW(wc.lpszClassName, inst);
    if (SUCCEEDED(co)) CoUninitialize();
    // the settings as set, for the game about to start or the next one
    app::Save();
    return ok && g_play;
}

}  // namespace launcher

// GameInfo::launcher: the game at once when its files are there, else (or with --configure) the Configuration app.
// As Savage Wind ships it, the release also has "Unchained Configuration.exe" (this executable under that name): it
// always opens the app, and PLAY starts Unchained.exe beside it.
bool UnchainedLauncher() {
    if (std::getenv("XBE_AUTOSTART")) return true;  // unattended test runs
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    bool config_exe = std::wcsstr(std::filesystem::path(self).filename().c_str(), L"Configuration") != nullptr;
    bool configure = config_exe || std::wcsstr(GetCommandLineW(), L"--configure") != nullptr;
    bool present = launcher::GameFilesPresent();
    if (!configure && present) return true;
    bool play = launcher::Run(!present);
    if (!play || !config_exe) return play;
    // the Configuration app's PLAY: the game is the other executable
    std::filesystem::path game = std::filesystem::path(self).parent_path() / L"Unchained.exe";
    STARTUPINFOW si{sizeof(si)};
    PROCESS_INFORMATION pi{};
    std::wstring cmd = L"\"" + game.wstring() + L"\"";
    if (CreateProcessW(game.c_str(), cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, game.parent_path().c_str(), &si,
                       &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    return false;
}
