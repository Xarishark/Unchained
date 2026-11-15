// Hooks for default.xbe. A strong definition replaces the recompiled function at that address.

#include "xdk/hle.h"
#include "core/log.h"
#include "kernel/kernel.h"

using kernel::R32;

// Bring-up diagnostics (release builds only log Error and above, so these use LOGE):
// the render device's push buffers come back null. Logs the game's push buffer maker and its XMemAlloc.

X86_EXTERN_FUNC(__imp__sub_002B7010);
X86_FUNC(sub_002B7010) {  // push buffer object: (size, with_memory) -> D3DPushBuffer*
    static int n = 0;
    uint32_t size = R32(ctx.esp + 4), with_memory = R32(ctx.esp + 8);
    __imp__sub_002B7010(ctx, base);
    if (n++ < 8) LOGE("unchained: push buffer maker(size %X, memory %u) = %08X", size, with_memory, ctx.eax);
}

X86_EXTERN_FUNC(__imp__sub_00026670);
X86_FUNC(sub_00026670) {  // the game's XMemAlloc(size, attributes)
    static int n = 0;
    uint32_t size = R32(ctx.esp + 4), attributes = R32(ctx.esp + 8);
    __imp__sub_00026670(ctx, base);
    if (n++ < 12 || !ctx.eax) LOGE("unchained: XMemAlloc(%X, %08X) = %08X", size, attributes, ctx.eax);
}

X86_EXTERN_FUNC(__imp__sub_0013276D);
X86_FUNC(sub_0013276D) {  // XMemAllocDefault(size, attributes), the fallback before the game's heap is up
    static int n = 0;
    uint32_t size = R32(ctx.esp + 4), attributes = R32(ctx.esp + 8);
    __imp__sub_0013276D(ctx, base);
    if (n++ < 12 || !ctx.eax) LOGE("unchained: XMemAllocDefault(%X, %08X) = %08X", size, attributes, ctx.eax);
}
