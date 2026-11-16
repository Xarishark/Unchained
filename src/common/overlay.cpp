// The performance overlay (TODO.md item 26), in the game's window over the picture: F10 shows or hides it,
// perf_overlay starts it shown. Frame rate and time, a graph of the last frames' times, the 1% and 0.1% lows and
// hitches over 25 and 50 ms of the last 1000 frames, and the render resolution. Drawn in the Configuration app's
// terminal green.
#include "app/app.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <imgui.h>

bool g_perf_overlay = false;

namespace {

constexpr size_t kGraph = 240;   // frames in the graph
constexpr size_t kWindow = 1000; // frames the lows and hitches count over
std::vector<float> g_times(kWindow, 0.0f);
size_t g_next = 0, g_count = 0;
std::chrono::steady_clock::time_point g_last{};

constexpr ImU32 kGreen = IM_COL32(57, 242, 138, 255);
constexpr ImU32 kGreenDim = IM_COL32(31, 143, 85, 255);
constexpr ImU32 kRed = IM_COL32(224, 52, 58, 255);
constexpr ImU32 kPanel = IM_COL32(6, 30, 32, 210);

float Low(std::vector<float> sorted_desc_src, double share) {
    size_t n = std::min(g_count, kWindow);
    if (!n) return 0;
    std::vector<float> t(sorted_desc_src.begin(), sorted_desc_src.begin() + n);
    std::sort(t.begin(), t.end(), std::greater<float>());
    size_t k = std::max<size_t>(1, size_t(n * share));
    double sum = 0;
    for (size_t i = 0; i < k; i++) sum += t[i];
    return sum > 0 ? float(1000.0 * k / sum) : 0;
}

}  // namespace

// window thread: F10 toggles
bool OverlayMessage(uint32_t msg, uintptr_t wp) {
    if (msg == WM_KEYDOWN && wp == VK_F10) {
        g_perf_overlay = !g_perf_overlay;
        return true;
    }
    if (msg == WM_SYSKEYDOWN && wp == VK_F10) {  // F10 is a system key: it would open the window menu
        g_perf_overlay = !g_perf_overlay;
        return true;
    }
    return false;
}

// each frame inside ImGui's (GameInfo::ui)
void OverlayUi(double fps) {
    auto now = std::chrono::steady_clock::now();
    if (g_last.time_since_epoch().count()) {
        g_times[g_next] = std::chrono::duration<float, std::milli>(now - g_last).count();
        g_next = (g_next + 1) % kWindow;
        g_count++;
    }
    g_last = now;
    if (!g_perf_overlay) return;

    size_t n = std::min(g_count, kWindow);
    int over25 = 0, over50 = 0;
    for (size_t i = 0; i < n; i++) {
        over25 += g_times[i] > 25;
        over50 += g_times[i] > 50;
    }
    float last = g_times[(g_next + kWindow - 1) % kWindow];
    float low1 = Low(g_times, 0.01), low01 = Low(g_times, 0.001);

    ImGui::SetNextWindowPos(ImVec2(12, 12), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0);
    ImGui::Begin("##perf", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    // the panel behind the text: ImGui's background list, still over the game's picture
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 at = ImGui::GetCursorScreenPos();
    float w = 300, gh = 70;
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(kGreen));
    ImGui::Text("%5.1f fps   %5.2f ms", fps, last);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(kGreenDim));
    ImGui::Text("1%% low %5.1f   0.1%% low %5.1f", low1, low01);
    ImGui::Text("hitches >25 ms %d   >50 ms %d   (last %zu)", over25, over50, n);
    app::Mode out = app::Output();
    float scale = app::RenderScale(640, 480);
    ImGui::Text("render %ux%u  scale %.2f", uint32_t(640 * scale), uint32_t(480 * scale), scale);
    ImGui::PopStyleColor();
    // the graph: one bar per frame, 16.7 ms at a third of its height, red past 25 ms
    ImVec2 g0 = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, gh));
    dl->AddRectFilled(ImVec2(at.x - 6, at.y - 4), ImVec2(g0.x + w + 6, g0.y + gh + 6), kPanel);
    dl->AddRect(ImVec2(at.x - 6, at.y - 4), ImVec2(g0.x + w + 6, g0.y + gh + 6), kGreen);
    float ref = g0.y + gh - gh / 3;
    dl->AddLine(ImVec2(g0.x, ref), ImVec2(g0.x + w, ref), kGreenDim);
    size_t shown = std::min(n, kGraph);
    for (size_t i = 0; i < shown; i++) {
        float t = g_times[(g_next + kWindow - shown + i) % kWindow];
        float hgt = std::min(gh, t / 50.0f * gh);
        float x = g0.x + w * float(i) / float(kGraph);
        dl->AddLine(ImVec2(x, g0.y + gh), ImVec2(x, g0.y + gh - hgt), t > 25 ? kRed : kGreen);
    }
    ImGui::PopStyleColor();
    (void)out;
    ImGui::End();
}
