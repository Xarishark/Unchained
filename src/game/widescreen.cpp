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

X86_FUNC(sub_001390C0) {
    constexpr float k43 = 4.0f / 3.0f;
    constexpr float kNear43 = 0.001f;  // 1.3330 and 1.3333334 are both the game's 4:3
    float k = app::WideFactor();
    uint32_t at = ctx.esp + 8;  // [esp] return, [esp+4] fov, [esp+8] aspect
    float aspect;
    std::memcpy(&aspect, kernel::HostPtr(at), 4);
    if (k != 1.0f && std::fabs(aspect - k43) < kNear43) {
        float wide = aspect * k;
        std::memcpy(kernel::HostPtr(at), &wide, 4);
    }
    __imp__sub_001390C0(ctx, base);
}
