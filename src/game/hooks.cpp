// Hooks for default.xbe. A strong definition replaces the recompiled function at that address, for example:
//
// #include "xdk/hle.h"
//
// X86_EXTERN_FUNC(__imp__sub_00012345);
// X86_FUNC(sub_00012345) {
//     __imp__sub_00012345(ctx, base);  // the original
// }
//
// Release builds log only Error and above unless UNCHAINED_VERBOSE_LOG is on (CMakeLists.txt); with it,
// [xbe] trace = [...] in config/game.toml logs a function's arguments and result without a hook.
