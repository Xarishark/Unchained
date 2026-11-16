// Hooks for default.xbe. A strong definition replaces the recompiled function at that address.
//
// Release builds log only Error and above unless UNCHAINED_VERBOSE_LOG is on (CMakeLists.txt); with it,
// [xbe] trace = [...] in config/game.toml logs a function's arguments and result without a hook.

#include "xdk/hle.h"
#include "core/log.h"
#include "kernel/kernel.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <string>

// The game's debug print (cdecl, printf style): retail builds send it nowhere. Its messages go to the log, each
// format at most 50 times, as the game's own account of what it skips ("SND: Start on not allocated channel!").
X86_FUNC(sub_001ED470) {
    using kernel::R32;
    auto guest_str = [](uint32_t va, size_t max) {
        std::string s;
        if (!va) return std::string("(null)");
        for (size_t i = 0; i < max; i++) {
            char c = static_cast<const char*>(kernel::HostPtr(va))[i];
            if (!c) break;
            s += c;
        }
        return s;
    };
    uint32_t fmt_va = R32(ctx.esp + 4);
    std::string fmt = guest_str(fmt_va, 400);
    static std::map<uint32_t, int> seen;
    if (seen[fmt_va]++ < 50) {
        std::string out;
        uint32_t arg = ctx.esp + 8;
        for (size_t i = 0; i < fmt.size(); i++) {
            if (fmt[i] != '%') {
                out += fmt[i];
                continue;
            }
            size_t j = i + 1;
            while (j < fmt.size() && std::strchr("-+ #0123456789.lh", fmt[j])) j++;
            if (j >= fmt.size()) break;
            std::string spec = fmt.substr(i, j - i + 1);
            char conv = fmt[j], buf[512];
            if (conv == '%') {
                out += '%';
            } else if (conv == 's') {
                out += guest_str(R32(arg), 256);
                arg += 4;
            } else if (conv == 'f' || conv == 'g' || conv == 'e') {
                double d;
                std::memcpy(&d, kernel::HostPtr(arg), 8);
                std::snprintf(buf, sizeof(buf), spec.c_str(), d);
                out += buf;
                arg += 8;
            } else {
                std::string s32 = spec;
                std::snprintf(buf, sizeof(buf), s32.c_str(), R32(arg));
                out += buf;
                arg += 4;
            }
            i = j;
        }
        while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
        LOGI("game: %s", out.c_str());
    }
    ctx.eax = 0;
    ctx.esp += 4;  // cdecl: the caller pops the arguments
}

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
