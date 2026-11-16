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

X86_FUNC(sub_001390C0) {
    constexpr float k43 = 4.0f / 3.0f;
    constexpr float kNear43 = 0.001f;  // 1.3330 and 1.3333334 are both the game's 4:3
    constexpr float kGameFov = 1.0471976f;
    constexpr float kSameFov = 1e-4f;
    constexpr uint32_t kMinFov = 40, kMaxFov = 110;
    uint32_t fov_at = ctx.esp + 4;
    float fov;
    std::memcpy(&fov, kernel::HostPtr(fov_at), 4);
    if (g_fov_degrees != 60 && std::fabs(fov - kGameFov) < kSameFov) {
        uint32_t deg = g_fov_degrees < kMinFov ? kMinFov : g_fov_degrees > kMaxFov ? kMaxFov : g_fov_degrees;
        fov = float(deg) * 3.14159265f / 180.0f;
        std::memcpy(kernel::HostPtr(fov_at), &fov, 4);
    }
    float k = app::WideFactor();
    uint32_t at = ctx.esp + 8;  // [esp] return, [esp+4] fov, [esp+8] aspect
    float aspect;
    std::memcpy(&aspect, kernel::HostPtr(at), 4);
    bool camera = std::fabs(aspect - k43) < kNear43;  // the game's 4:3 views, not a light's square one
    if (k != 1.0f && camera) {
        float wide = aspect * k;
        std::memcpy(kernel::HostPtr(at), &wide, 4);
    }
    // draw_distance: the cameras' far clip, percent of the game's ([esp+0x10]); what the game culls itself stays
    constexpr uint32_t kMaxDistance = 1000;
    if (camera && g_draw_distance != 100) {
        uint32_t far_at = ctx.esp + 0x10;
        float far_clip;
        std::memcpy(&far_clip, kernel::HostPtr(far_at), 4);
        float pct = float(g_draw_distance < 100 ? 100 : g_draw_distance > kMaxDistance ? kMaxDistance : g_draw_distance);
        far_clip *= pct / 100.0f;
        std::memcpy(kernel::HostPtr(far_at), &far_clip, 4);
    }
    __imp__sub_001390C0(ctx, base);
}
