// Hooks for default.xbe. A strong definition replaces the recompiled function at that address.
//
// Release builds log only Error and above unless UNCHAINED_VERBOSE_LOG is on (CMakeLists.txt); with it,
// [xbe] trace = [...] in config/game.toml logs a function's arguments and result without a hook.

#include "xdk/hle.h"
#include "core/log.h"

// XAPI's process start checks that the disc is an authentic Xbox disc: it opens the DVD drive and reads the disc
// security mode page (SCSI MODE SENSE 0x5A, page 0x3E) through IOCTL 0x4D014. The answer the runtime gives fails it,
// so XAPI asks to reboot to the dashboard (refused) and skips the rest of its start, which mounts U:, the title's save
// drive. The game then fails to create player data ("a hard disk error occurred"). The player runs their own retail
// disc here, so the check passes, as the xiso tool's one byte patch of its caller (0x133921) makes it pass.
X86_FUNC(sub_001336E9) {
    static bool logged = false;
    if (!logged) {
        logged = true;
        LOGI("unchained: disc authentication check passes (XAPI start, 0x1336E9)");
    }
    ctx.eax = 0;  // STATUS_SUCCESS: an authentic disc
    ctx.esp += 4;  // pop the return address: stdcall, no arguments
}
