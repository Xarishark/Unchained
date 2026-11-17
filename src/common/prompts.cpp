// Button prompts for the device in use (TODO.md item 15). The game's prompts are characters of its font: the
// coloured A B X Y discs, White, Black and the triggers sit in one row of the font texture. Those cells are drawn
// again here for the controller in use (a modern Xbox pad's lettered buttons and LB RB LT RT, PlayStation's symbols,
// Nintendo's letters) or the keyboard's bound keys, shapes and key caps made in code (no art), through
// GameInfo::texture_image, and drawn anew whenever the device in use changes.
//
// button_prompts: 0 the device in use, 1 Xbox, 2 PlayStation, 3 Nintendo, 4 keyboard, 5 the console's own glyphs.
#define NOMINMAX
#include "app/app.h"
#include "core/input.h"
#include "core/log.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <windows.h>

uint32_t g_button_prompts = 0;

std::string BoundKeyName(const char* action);  // keyboard.cpp: the action's first key, "" when unbound

namespace {

enum Shown : int { kXbox, kPlayStation, kNintendo, kKeyboard, kConsole };

constexpr uint64_t kFontKey = 0x1a4d811ab994c436;  // the font texture (256x256 DXT5), as the texture dumps name it
constexpr uint32_t kFontSide = 256;
constexpr uint32_t kScale = 4;  // the edited font is drawn 4x for crisp glyphs
// the button row in the font (texels, rows top down as stored: the font is upside down)
constexpr int kRowTop = 66, kRowBottom = 85;
enum Cell { kA, kB, kX, kY, kWhite, kBlack, kLt, kRt, kCells };
constexpr int kCellLeft[kCells] = {61, 1, 21, 41, 89, 151, 116, 185};
constexpr int kCellRight[kCells] = {78, 18, 38, 58, 103, 165, 135, 204};
// the keyboard action each cell's button is bound to
const char* const kAction[kCells] = {"jump", "b", "attack", "y", "white", "black", "left_trigger", "right_trigger"};

std::atomic<int> g_device{kXbox};  // the device in use: Shown
std::atomic<int> g_applied{-1};    // the glyphs drawn last

int ShownNow() {
    switch (g_button_prompts) {
        case 1: return kXbox;
        case 2: return kPlayStation;
        case 3: return kNintendo;
        case 4: return kKeyboard;
        case 5: return kConsole;
        default: return g_device.load();
    }
}

// straight alpha RGBA floats, 0 to 1
struct Canvas {
    int w, h;
    std::vector<float> px;
    Canvas(int w_, int h_) : w(w_), h(h_), px(size_t(w_) * h_ * 4, 0.0f) {}
    void Put(int x, int y, const float c[4], float cover) {
        if (x < 0 || y < 0 || x >= w || y >= h || cover <= 0) return;
        float* d = &px[(size_t(y) * w + x) * 4];
        float a = c[3] * std::min(cover, 1.0f);
        float out = a + d[3] * (1 - a);
        for (int i = 0; i < 3; i++) d[i] = out > 0 ? (c[i] * a + d[i] * d[3] * (1 - a)) / out : 0;
        d[3] = out;
    }
    // a shape by its signed distance (negative inside), anti-aliased over a pixel
    template <class F>
    void Fill(const float c[4], F dist) {
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) Put(x, y, c, std::clamp(0.5f - dist(x + 0.5f, y + 0.5f), 0.0f, 1.0f));
    }
};

float Segment(float px, float py, float ax, float ay, float bx, float by) {
    float vx = bx - ax, vy = by - ay;
    float t = std::clamp(((px - ax) * vx + (py - ay) * vy) / (vx * vx + vy * vy), 0.0f, 1.0f);
    return std::hypot(px - ax - vx * t, py - ay - vy * t);
}

float RoundBox(float px, float py, float cx, float cy, float hw, float hh, float r) {
    float qx = std::abs(px - cx) - hw + r, qy = std::abs(py - cy) - hh + r;
    return std::hypot(std::max(qx, 0.0f), std::max(qy, 0.0f)) + std::min(std::max(qx, qy), 0.0f) - r;
}

// text's coverage from GDI, white on black, centred in the canvas
void Text(Canvas& cv, const std::wstring& s, const float c[4], float height, float max_w) {
    HDC dc = CreateCompatibleDC(nullptr);
    BITMAPINFO bi{};
    bi.bmiHeader = {sizeof(BITMAPINFOHEADER), cv.w, -cv.h, 1, 32, BI_RGB};
    void* bits = nullptr;
    HBITMAP bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dc || !bmp) {
        if (bmp) DeleteObject(bmp);
        if (dc) DeleteDC(dc);
        return;
    }
    HGDIOBJ old_bmp = SelectObject(dc, bmp);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    // the largest size up to height that fits max_w
    HFONT font = nullptr;
    for (int size = int(height); size >= 6; size--) {
        if (font) DeleteObject(font);
        font = CreateFontW(-size, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                           ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Bahnschrift");
        SelectObject(dc, font);
        SIZE ext{};
        GetTextExtentPoint32W(dc, s.c_str(), int(s.size()), &ext);
        if (ext.cx <= max_w) break;
    }
    RECT rc{0, 0, cv.w, cv.h};
    DrawTextW(dc, s.c_str(), int(s.size()), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    GdiFlush();
    const uint8_t* p = static_cast<const uint8_t*>(bits);
    for (int y = 0; y < cv.h; y++)
        for (int x = 0; x < cv.w; x++) {
            const uint8_t* q = p + (size_t(y) * cv.w + x) * 4;
            cv.Put(x, y, c, std::max({q[0], q[1], q[2]}) / 255.0f);
        }
    SelectObject(dc, old_bmp);
    if (font) DeleteObject(font);
    DeleteObject(bmp);
    DeleteDC(dc);
}

constexpr float kDark[4] = {0.10f, 0.10f, 0.11f, 1};
constexpr float kRim[4] = {0.62f, 0.64f, 0.66f, 1};
constexpr float kWhiteInk[4] = {0.96f, 0.96f, 0.96f, 1};
constexpr float kKeyCap[4] = {0.86f, 0.87f, 0.88f, 1};
constexpr float kKeyEdge[4] = {0.35f, 0.36f, 0.38f, 1};
constexpr float kKeyInk[4] = {0.08f, 0.08f, 0.09f, 1};

// a face button: a dark disc with a light rim, as the console's discs sit in the font
void Disc(Canvas& cv) {
    float cx = cv.w / 2.0f, cy = cv.h / 2.0f, r = std::min(cv.w, cv.h) * 0.47f;
    cv.Fill(kRim, [&](float x, float y) { return std::hypot(x - cx, y - cy) - r; });
    cv.Fill(kDark, [&](float x, float y) { return std::hypot(x - cx, y - cy) - r * 0.86f; });
}

// PlayStation's symbols in their colours
void PlayStationFace(Canvas& cv, Cell cell) {
    Disc(cv);
    float cx = cv.w / 2.0f, cy = cv.h / 2.0f, s = std::min(cv.w, cv.h) * 0.22f, t = s * 0.36f;
    static constexpr float kCross[4] = {0.49f, 0.69f, 0.91f, 1}, kCircle[4] = {1.0f, 0.40f, 0.40f, 1},
                           kSquare[4] = {0.91f, 0.59f, 0.82f, 1}, kTriangle[4] = {0.25f, 0.88f, 0.69f, 1};
    switch (cell) {
        case kA:
            cv.Fill(kCross, [&](float x, float y) {
                return std::min(Segment(x, y, cx - s, cy - s, cx + s, cy + s),
                                Segment(x, y, cx - s, cy + s, cx + s, cy - s)) - t / 2;
            });
            break;
        case kB: cv.Fill(kCircle, [&](float x, float y) { return std::abs(std::hypot(x - cx, y - cy) - s) - t / 2; }); break;
        case kX:
            cv.Fill(kSquare, [&](float x, float y) {
                return std::abs(std::max(std::abs(x - cx), std::abs(y - cy)) - s * 0.9f) - t / 2;
            });
            break;
        default: {  // triangle, point up
            float ax = cx, ay = cy - s * 1.05f, bx = cx - s * 1.1f, by = cy + s * 0.75f, ex = cx + s * 1.1f;
            cv.Fill(kTriangle, [&](float x, float y) {
                return std::min({Segment(x, y, ax, ay, bx, by), Segment(x, y, bx, by, ex, by),
                                 Segment(x, y, ex, by, ax, ay)}) - t / 2;
            });
            break;
        }
    }
}

// a shoulder or trigger: a dark rounded plate with its name
void Plate(Canvas& cv, const std::wstring& name) {
    float cx = cv.w / 2.0f, cy = cv.h / 2.0f, hw = cv.w * 0.47f, hh = cv.h * 0.36f, r = hh * 0.55f;
    cv.Fill(kRim, [&](float x, float y) { return RoundBox(x, y, cx, cy, hw, hh, r); });
    float in = std::min(cv.w, cv.h) * 0.06f;
    cv.Fill(kDark, [&](float x, float y) { return RoundBox(x, y, cx, cy, hw - in, hh - in, r - in); });
    Text(cv, name, kWhiteInk, hh * 1.25f, (hw - in) * 1.8f);
}

// a key cap with the bound key's name
void KeyCap(Canvas& cv, const std::string& key) {
    static const std::pair<const char*, const wchar_t*> kShort[] = {
        {"Space", L"Space"}, {"Enter", L"Enter"}, {"Escape", L"Esc"}, {"Backspace", L"Bksp"}, {"LShift", L"Shift"},
        {"RShift", L"Shift"}, {"LCtrl", L"Ctrl"}, {"RCtrl", L"Ctrl"}, {"LAlt", L"Alt"}, {"RAlt", L"Alt"},
        {"Mouse1", L"LMB"}, {"Mouse2", L"RMB"}, {"Mouse3", L"MMB"}, {"Mouse4", L"M4"}, {"Mouse5", L"M5"},
        {"Up", L"↑"}, {"Down", L"↓"}, {"Left", L"←"}, {"Right", L"→"}, {"Tab", L"Tab"}};
    std::wstring label;
    for (auto& [n, s] : kShort)
        if (key == n) label = s;
    if (label.empty()) label.assign(key.begin(), key.end());
    if (label.empty()) label = L"-";
    float cx = cv.w / 2.0f, cy = cv.h / 2.0f, hw = cv.w * 0.47f, hh = cv.h * 0.42f, r = cv.h * 0.12f;
    cv.Fill(kKeyEdge, [&](float x, float y) { return RoundBox(x, y, cx, cy, hw, hh, r); });
    float in = std::min(cv.w, cv.h) * 0.07f;
    cv.Fill(kKeyCap, [&](float x, float y) { return RoundBox(x, y, cx, cy - in * 0.4f, hw - in, hh - in, r); });
    Text(cv, label, kKeyInk, hh * 1.1f, (hw - in) * 1.85f);
}

// a modern Xbox pad's buttons: the letter in its colour on a dark disc
void XboxFace(Canvas& cv, Cell cell) {
    Disc(cv);
    static constexpr float kGreen[4] = {0.42f, 0.80f, 0.27f, 1}, kRed[4] = {0.93f, 0.28f, 0.24f, 1},
                           kBlue[4] = {0.25f, 0.58f, 0.95f, 1}, kYellow[4] = {0.98f, 0.78f, 0.18f, 1};
    const float* colour = cell == kA ? kGreen : cell == kB ? kRed : cell == kX ? kBlue : kYellow;
    const wchar_t* letter = cell == kA ? L"A" : cell == kB ? L"B" : cell == kX ? L"X" : L"Y";
    Text(cv, letter, colour, cv.h * 0.62f, cv.w * 0.8f);
}

// Nintendo's letters by position: south B, east A, west Y, north X
void NintendoFace(Canvas& cv, Cell cell) {
    Disc(cv);
    const wchar_t* letter = cell == kA ? L"B" : cell == kB ? L"A" : cell == kX ? L"Y" : L"X";
    Text(cv, letter, kWhiteInk, cv.h * 0.62f, cv.w * 0.8f);
}

void DrawCell(Canvas& cv, Cell cell, int shown) {
    bool face = cell <= kY;
    if (shown == kKeyboard) return KeyCap(cv, BoundKeyName(kAction[cell]));
    if (face) {
        if (shown == kPlayStation) return PlayStationFace(cv, cell);
        if (shown == kNintendo) return NintendoFace(cv, cell);
        return XboxFace(cv, cell);
    }
    // White and Black are the shoulder buttons on today's pads (the SDK maps them so)
    static const wchar_t* const kXs[] = {L"LB", L"RB", L"LT", L"RT"};
    static const wchar_t* const kPs[] = {L"L1", L"R1", L"L2", L"R2"};
    static const wchar_t* const kNs[] = {L"L", L"R", L"ZL", L"ZR"};
    Plate(cv, (shown == kPlayStation ? kPs : shown == kNintendo ? kNs : kXs)[cell - kWhite]);
}

}  // namespace

namespace {

// The game's popup icons: a 32x32 texture, a green box round a button's coloured disc (the Y icon that pops up at
// the top of the screen in the lab, and its siblings for the other buttons). Found by what is in them, since only the
// ones a run shows can be listed: the box's ring mostly green, and a bright disc of a button's colour inside.
constexpr uint32_t kIconSide = 32;

int IconButton(const std::vector<uint8_t>& bgra, uint32_t w) {
    uint32_t k = w / kIconSide;
    auto px = [&](uint32_t x, uint32_t y) { return &bgra[(size_t(y) * w + x) * 4]; };
    size_t ring = 0, green = 0;
    for (uint32_t y = 0; y < w; y++)
        for (uint32_t x = 0; x < w; x++) {
            if (std::min({x, y, w - 1 - x, w - 1 - y}) >= 2 * k) continue;
            const uint8_t* p = px(x, y);
            ring++;
            green += p[3] > 100 && p[1] > 140 && p[2] < 140 && p[1] > p[0];
        }
    constexpr double kGreenRing = 0.9;
    if (!ring || green < ring * kGreenRing) return -1;
    // the disc: the centre's bright, coloured texels, by their hue
    size_t n = 0, coloured = 0, white = 0;
    double hx = 0, hy = 0;
    for (uint32_t y = 10 * k; y < 22 * k; y++)
        for (uint32_t x = 10 * k; x < 22 * k; x++) {
            const uint8_t* p = px(x, y);
            if (p[3] < 128) continue;
            n++;
            float b = p[0] / 255.0f, g = p[1] / 255.0f, r = p[2] / 255.0f;
            float mx = std::max({r, g, b}), mn = std::min({r, g, b});
            float sat = mx > 0 ? (mx - mn) / mx : 0;
            if (mx > 0.65f && sat < 0.2f) white++;
            if (mx < 0.35f || sat < 0.35f) continue;
            float hue = mx == r ? std::fmod((g - b) / (mx - mn), 6.0f) : mx == g ? (b - r) / (mx - mn) + 2 : (r - g) / (mx - mn) + 4;
            hue *= 60;
            constexpr double kRad = 3.14159265358979 / 180;
            hx += std::cos(hue * kRad);
            hy += std::sin(hue * kRad);
            coloured++;
        }
    constexpr double kDiscShare = 0.3;
    if (n && coloured >= n * kDiscShare) {
        double hue = std::atan2(hy, hx) * 180 / 3.14159265358979;
        if (hue < 0) hue += 360;
        if (hue < 20 || hue >= 330) return kB;
        if (hue < 72) return kY;
        if (hue < 170) return kA;
        if (hue >= 180 && hue < 265) return kX;
        return -1;
    }
    if (n && white >= n * 0.4) return kWhite;
    return -1;
}

// a texture's level 0 4x, bilinear as the GPU samples it, on premultiplied colour
std::vector<uint8_t> Upscale4x(const std::vector<uint8_t>& bgra, uint32_t w, uint32_t h) {
    uint32_t W = w * kScale, H = h * kScale;
    std::vector<uint8_t> big(size_t(W) * H * 4);
    // bilinear, as the GPU samples the console's font, on premultiplied colour
    auto at = [&](int x, int y, int c) {
        x = std::clamp(x, 0, int(w) - 1);
        y = std::clamp(y, 0, int(h) - 1);
        const uint8_t* p = &bgra[(size_t(y) * w + x) * 4];
        return c == 3 ? p[3] / 255.0f : p[c] / 255.0f * (p[3] / 255.0f);
    };
    for (uint32_t y = 0; y < H; y++)
        for (uint32_t x = 0; x < W; x++) {
            float fx = (x + 0.5f) / kScale - 0.5f, fy = (y + 0.5f) / kScale - 0.5f;
            int x0 = int(std::floor(fx)), y0 = int(std::floor(fy));
            float tx = fx - x0, ty = fy - y0;
            float v[4];
            for (int c = 0; c < 4; c++) {
                float top = at(x0, y0, c) * (1 - tx) + at(x0 + 1, y0, c) * tx;
                float bot = at(x0, y0 + 1, c) * (1 - tx) + at(x0 + 1, y0 + 1, c) * tx;
                v[c] = top * (1 - ty) + bot * ty;
            }
            uint8_t* d = &big[(size_t(y) * W + x) * 4];
            for (int c = 0; c < 3; c++) d[c] = uint8_t(std::lround(std::clamp(v[3] > 0 ? v[c] / v[3] : 0.0f, 0.0f, 1.0f) * 255));
            d[3] = uint8_t(std::lround(v[3] * 255));
        }
    return big;
}

// a canvas over the picture (W wide) at x0, y0, its rows bottom up as the game's textures are stored
void PutFlipped(std::vector<uint8_t>& big, uint32_t W, const Canvas& cv, int x0, int y0) {
    int y1 = y0 + cv.h;
    for (int y = 0; y < cv.h; y++)
        for (int x = 0; x < cv.w; x++) {
            const float* s = &cv.px[(size_t(y) * cv.w + x) * 4];
            uint8_t* d = &big[(size_t(y1 - 1 - y) * W + (x0 + x)) * 4];
            float a = s[3], da = d[3] / 255.0f, out = a + da * (1 - a);
            for (int c = 0; c < 3; c++) {
                float sc = s[2 - c], dc = d[c] / 255.0f;
                d[c] = uint8_t(std::lround(std::clamp(out > 0 ? (sc * a + dc * da * (1 - a)) / out : 0.0f, 0.0f, 1.0f) * 255));
            }
            d[3] = uint8_t(std::lround(out * 255));
        }
}

}  // namespace

// texture_edits: the font, and any 32x32 texture that may be a popup icon (looked at on the worker)
bool PromptsEdits(uint64_t key, uint32_t w, uint32_t h) {
    return key == kFontKey || (w == kIconSide && h == kIconSide);
}

void DrawCells(std::vector<uint8_t>& big, uint32_t W, int shown);

// texture_image: the font 4x with its button cells drawn for the device shown, and the popup icons' buttons the same
// way (both left alone for the console's own)
bool PromptsImage(uint64_t key, uint32_t& w, uint32_t& h, std::vector<uint8_t>& bgra) {
    int shown = g_applied.load();
    if (shown < 0 || shown == kConsole || w != h) return false;
    if (key != kFontKey) {
        if (w != kIconSide && w != kIconSide * kScale) return false;
        int button = IconButton(bgra, w);
        if (button < 0) return false;
        std::vector<uint8_t> big = w == kIconSide ? Upscale4x(bgra, w, h) : bgra;
        uint32_t W = kIconSide * kScale;
        // the inside of the box: its own dark colour, then the button drawn for the device
        constexpr int kInset = 3;  // texels of the box's frame and margin
        int in0 = kInset * int(kScale), in1 = int(W) - kInset * int(kScale);
        const uint8_t* fill = &big[(size_t(in0) * W + in0) * 4];
        uint8_t back[4] = {fill[0], fill[1], fill[2], fill[3]};
        for (int y = in0; y < in1; y++)
            for (int x = in0; x < in1; x++) std::memcpy(&big[(size_t(y) * W + x) * 4], back, 4);
        Canvas cv(in1 - in0, in1 - in0);
        DrawCell(cv, Cell(button), shown);
        PutFlipped(big, W, cv, in0, in0);
        bgra = std::move(big);
        w = h = W;
        return true;
    }
    uint32_t W = kFontSide * kScale, H = W;
    // the SDK's ui_upscale has drawn the font 4x already; else it is upscaled here
    if (w == W) {
        DrawCells(bgra, W, shown);
        return true;
    }
    if (w != kFontSide) return false;
    std::vector<uint8_t> big = Upscale4x(bgra, w, h);
    DrawCells(big, W, shown);
    bgra = std::move(big);
    w = W;
    h = H;
    return true;
}

// the button row's cells, drawn into the 4x font (W wide)
void DrawCells(std::vector<uint8_t>& big, uint32_t W, int shown) {
    for (int cell = 0; cell < kCells; cell++) {
        int x0 = kCellLeft[cell] * int(kScale), x1 = (kCellRight[cell] + 1) * int(kScale);
        int y0 = kRowTop * int(kScale), y1 = (kRowBottom + 1) * int(kScale);
        Canvas cv(x1 - x0, y1 - y0);
        DrawCell(cv, Cell(cell), shown);
        // the font is stored upside down: the glyph's rows go in bottom up
        for (int y = 0; y < cv.h; y++)
            for (int x = 0; x < cv.w; x++) {
                const float* s = &cv.px[(size_t(y) * cv.w + x) * 4];
                uint8_t* d = &big[(size_t(y1 - 1 - y) * W + (x0 + x)) * 4];
                d[0] = uint8_t(std::lround(s[2] * 255));
                d[1] = uint8_t(std::lround(s[1] * 255));
                d[2] = uint8_t(std::lround(s[0] * 255));
                d[3] = uint8_t(std::lround(s[3] * 255));
            }
    }
}

// port 0's poll, before the keyboard is added: a pad in use shows its maker's glyphs
void PromptsPad(const input::Pad& p) {
    constexpr int kStickDead = 12000;
    constexpr uint8_t kPressed = 64;
    static bool first = true;
    bool used = p.buttons || std::any_of(std::begin(p.analog), std::end(p.analog), [](uint8_t a) { return a > kPressed; }) ||
                std::abs(p.lx) > kStickDead || std::abs(p.ly) > kStickDead || std::abs(p.rx) > kStickDead ||
                std::abs(p.ry) > kStickDead;
    if (first) {
        // at start: the controller if one is plugged in, else the keyboard
        g_device = p.connected ? p.family : kKeyboard;
        first = false;
    } else if (p.connected && used) {
        g_device = p.family;
    }
    // tests: UNCHAINED_PROMPTS_SWITCH="seconds:button_prompts,..." changes the setting during the run
    static const std::vector<std::pair<double, uint32_t>> switches = [] {
        std::vector<std::pair<double, uint32_t>> out;
        if (const char* e = std::getenv("UNCHAINED_PROMPTS_SWITCH")) {
            for (const char* q = e; *q;) {
                char* end = nullptr;
                double t = std::strtod(q, &end);
                if (end == q || *end != ':') break;
                uint32_t v = uint32_t(std::strtoul(end + 1, &end, 10));
                out.push_back({t, v});
                q = *end == ',' ? end + 1 : end;
            }
        }
        return out;
    }();
    static const auto start = std::chrono::steady_clock::now();
    double now = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    for (auto& [t, v] : switches)
        if (now >= t) g_button_prompts = v;
    int shown = ShownNow();
    if (g_applied.exchange(shown) != shown) {
        static const char* const kNames[] = {"Xbox", "PlayStation", "Nintendo", "keyboard", "the console's"};
        LOGI("unchained: %s button prompts", kNames[shown]);
        app::RefreshTextureImages();
    }
}

// the keyboard or mouse in use
void PromptsKeyboard() { g_device = kKeyboard; }
