// Widescreen (app::Wide::Expand): Whiplash builds every perspective projection in one function, 0x1390C0
// (thiscall: the matrix in ecx, then fov, aspect, near and far as floats, ret 0x10). Its fov is vertical: m00 is
// near / (aspect * near * tan(fov / 2)) and m11 near / (near * tan(fov / 2)), so a wider aspect alone shows more at
// the sides with the same height (Hor+). Its callers pass the game's 4:3 (1.3333334 from the camera made at 0x17F3C0,
// 1.3330 at 0x23FF0A); the hook widens exactly those by app::WideFactor and leaves any other aspect (a square light
// view) alone. The renderer shows the 640x448 view at the output's aspect and narrows the 2D back to 4:3.

#include "app/app.h"
#include "core/log.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

X86_EXTERN_FUNC(__imp__sub_001390C0);

// fov: the gameplay camera's vertical field of view in degrees (the game's is 60, 1.0471976 rad at 0x30C0AC). Only a
// projection at exactly that angle changes, so cutscene cameras with their own angle keep it.
uint32_t g_fov_degrees = 60;
uint32_t g_draw_distance = 100;  // percent of the game's far clip, 100 to 1000

// The camera's aspect getter (0x17F380, fld [ecx+0x50]; ret): the render setup copies fov, aspect, near and far from
// the camera into the frame's view (0x16A2BB), which the projection and the game's own visibility tests read. With
// the 4:3 aspect there, objects coming in at the widened edges were culled until they reached the 4:3 view and
// popped in. Widened here, the view and its culling cover what is shown; the projection hook leaves an aspect that is
// no longer 4:3 alone, so nothing widens twice.
X86_FUNC(sub_0017F380) {
    constexpr float k43 = 4.0f / 3.0f;
    constexpr float kNear43 = 0.001f;
    float aspect;
    std::memcpy(&aspect, kernel::HostPtr(ctx.ecx + 0x50), 4);
    float k = app::WideFactor();
    if (k != 1.0f && std::fabs(aspect - k43) < kNear43) aspect *= k;
    x87_push(ctx, double(aspect));
    ctx.esp += 4;  // ret
}

namespace {
constexpr float k43 = 4.0f / 3.0f;
constexpr float kNear43 = 0.001f;  // 1.3330 and 1.3333334 are both the game's 4:3

// fov setting: the gameplay camera's 60 degrees becomes g_fov_degrees; any other angle (cutscenes) stays
float GameFov(float fov) {
    constexpr float kGameFov = 1.0471976f;
    constexpr float kSameFov = 1e-4f;
    constexpr uint32_t kMinFov = 40, kMaxFov = 110;
    if (g_fov_degrees == 60 || std::fabs(fov - kGameFov) >= kSameFov) return fov;
    uint32_t deg = g_fov_degrees < kMinFov ? kMinFov : g_fov_degrees > kMaxFov ? kMaxFov : g_fov_degrees;
    return float(deg) * 3.14159265f / 180.0f;
}

// draw_distance setting: a camera's far clip, percent of the game's
float FarClip(float far_clip) {
    constexpr uint32_t kMaxDistance = 1000;
    if (g_draw_distance == 100) return far_clip;
    uint32_t pct = g_draw_distance < 100 ? 100 : g_draw_distance > kMaxDistance ? kMaxDistance : g_draw_distance;
    return far_clip * float(pct) / 100.0f;
}

float ReadF(uint32_t at) {
    float f;
    std::memcpy(&f, kernel::HostPtr(at), 4);
    return f;
}
void WriteF(uint32_t at, float f) { std::memcpy(kernel::HostPtr(at), &f, 4); }

// a projection draw_distance applies to: the game's 4:3 aspect as given (an aspect the getter above widened
// already keeps the game's far clip; the frustum below follows the same rule so the two agree)
bool FarScaled(float aspect) { return std::fabs(aspect - k43) < kNear43; }
}  // namespace

X86_FUNC(sub_001390C0) {
    // [esp] return, [esp+4] fov, [esp+8] aspect, [esp+0xC] near, [esp+0x10] far
    WriteF(ctx.esp + 4, GameFov(ReadF(ctx.esp + 4)));
    float aspect = ReadF(ctx.esp + 8);
    bool camera = std::fabs(aspect - k43) < kNear43;  // an aspect widened already (the getter above) stays
    float k = app::WideFactor();
    if (k != 1.0f && camera) WriteF(ctx.esp + 8, aspect * k);
    if (FarScaled(aspect)) WriteF(ctx.esp + 0x10, FarClip(ReadF(ctx.esp + 0x10)));
    __imp__sub_001390C0(ctx, base);
}

// The renderer's cull frustum (0x16F620, thiscall on the renderer, ret 4) builds six planes from the view it was
// given: fov at +0x518, aspect +0x51C, near +0x510, far +0x514. Its sphere test (0x16D680) checks every room
// drawable against them (0x194200). It reads the camera's own fov and far, not the projection's, so with fov above
// 60 whatever lay between the game's view and the one drawn was culled: wall lights, plants, the planter and palm,
// hanging platforms, the lab's crystal popped in and out at the edges as the camera turned. The frustum is built
// with the projection's fov and far, then the view's own values go back. (Its aspect is the projection's already,
// from the camera's aspect getter above.) UNCHAINED_OLD_FRUSTUM=1 leaves the game's own frustum, for A/B runs.
X86_EXTERN_FUNC(__imp__sub_0016F620);
static const bool g_old_frustum = std::getenv("UNCHAINED_OLD_FRUSTUM") != nullptr;
X86_FUNC(sub_0016F620) {
    if (g_old_frustum) {
        __imp__sub_0016F620(ctx, base);
        return;
    }
    uint32_t r = ctx.ecx;
    float fov = ReadF(r + 0x518), far_clip = ReadF(r + 0x514);
    WriteF(r + 0x518, GameFov(fov));
    if (FarScaled(ReadF(r + 0x51C))) WriteF(r + 0x514, FarClip(far_clip));
    __imp__sub_0016F620(ctx, base);
    WriteF(r + 0x518, fov);
    WriteF(r + 0x514, far_clip);
}
