// Culling and light beam probe from the widescreen pop-in work (2026-11-16); see docs/LIGHT_BEAM_FLICKER.md.
// Not built: copy it to src/game/ (the game library globs src/game/*.cpp), build, and run with any of
//   UNCHAINED_DBG_CULL=1     the renderer sphere test's counts and its frustum's left/right plane angle
//   UNCHAINED_DBG_NOCULL=1   the sphere test (0x16D680) passes everything
//   UNCHAINED_DBG_BEAM=1     the light beams: ray test (0xDCA10) and what it hit (0x142A00), fade (0xDCD30), the
//                            time between a beam's draws (0xDD060), and when a beam's drawable meets the sphere test
//   UNCHAINED_DBG_PORTALS=1  every portal test (0x1BC980) reports "all visible"
// Lines go to the game log as "cull:", "beam", "beamdraw" and "beamtest". Remove it from src/game afterwards.

#include "core/log.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"

#include <cmath>
#include <cstdlib>
#include <cstring>


X86_EXTERN_FUNC(__imp__sub_0016D680);

namespace {
bool Env(const char* n) {
    const char* v = std::getenv(n);
    return v && *v && *v != '0';
}
float F(uint32_t a) {
    float f;
    std::memcpy(&f, kernel::HostPtr(a), 4);
    return f;
}
const bool g_log = Env("UNCHAINED_DBG_CULL");
const bool g_nocull = Env("UNCHAINED_DBG_NOCULL");
uint32_t g_frustums = 0, g_tests = 0, g_culled = 0;
}  // namespace

bool BeamDrawableTested(uint32_t center, uint32_t culled);
X86_FUNC(sub_0016D680) {
    uint32_t center = 0;
    std::memcpy(&center, kernel::HostPtr(ctx.esp + 4), 4);
    __imp__sub_0016D680(ctx, base);
    BeamDrawableTested(center, ctx.eax & 0xFF);
    ++g_tests;
    if (ctx.eax & 0xFF) ++g_culled;
    if (g_nocull) ctx.eax &= ~0xFFu;
    if (g_log && g_tests % 20000 == 0) {
        float a[3] = {F(0x37F8D8), F(0x37F8E8), F(0x37F8F8)}, b[3] = {F(0x37F8DC), F(0x37F8EC), F(0x37F8FC)};
        float d = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
        float la = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]), lb = std::sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
        LOGR("cull: tests %u culled %u, planes 2-3 %.1f deg", g_tests, g_culled, 180.0f - std::acos(d / (la * lb)) * 57.29578f);
    }
}

// beam probe (UNCHAINED_DBG_BEAM=1): 0xDCA10, the light beam's ray test (thiscall, start ptr, end ptr, length;
// ret 0xC), and 0xDCD30, its fade (ret 0x14, [esp+8] the float it writes). Logs a beam when a value changes.
X86_EXTERN_FUNC(__imp__sub_000DCA10);
X86_EXTERN_FUNC(__imp__sub_000DCD30);
#include <unordered_map>
namespace {
const bool g_beam = Env("UNCHAINED_DBG_BEAM");
struct BeamState { int hit = -1; float len = -1, fade = -2; };
std::unordered_map<uint32_t, BeamState> g_beams;
uint32_t Frame() { uint32_t f; std::memcpy(&f, kernel::HostPtr(0x36320C), 4); return f; }
}  // namespace

// 0x142A00 (cdecl ray cast: from, to, ?, hit info, mask, 2) called from the beam's test: the first cast (any
// object, back at 0xDCABE) and the re-cast against mask 0x8000 (back at 0xDCB57) when the hit object was not drawn
X86_EXTERN_FUNC(__imp__sub_00142A00);
namespace { uint32_t g_c1obj, g_c1hit, g_c2 , g_c2hit, g_c2obj; }
X86_FUNC(sub_00142A00) {
    uint32_t ret, info = 0;
    std::memcpy(&ret, kernel::HostPtr(ctx.esp), 4);
    bool mine = g_beam && (ret == 0xDCABE || ret == 0xDCB57);
    if (mine) std::memcpy(&info, kernel::HostPtr(ctx.esp + 0x10), 4);
    __imp__sub_00142A00(ctx, base);
    if (!mine || !info) return;
    uint32_t obj;
    std::memcpy(&obj, kernel::HostPtr(info), 4);
    if (ret == 0xDCABE) { g_c1hit = ctx.eax & 0xFF; g_c1obj = obj; g_c2 = 0; }
    if (ret == 0xDCB57) { g_c2 = 1; g_c2hit = ctx.eax & 0xFF; g_c2obj = obj; }
}

X86_FUNC(sub_000DCA10) {
    uint32_t self = ctx.ecx, s = 0, e = 0;
    float len = 0, before[3] = {};
    if (g_beam) {
        std::memcpy(&s, kernel::HostPtr(ctx.esp + 4), 4);
        std::memcpy(&e, kernel::HostPtr(ctx.esp + 8), 4);
        len = F(ctx.esp + 0xC);
        for (int i = 0; i < 3; ++i) before[i] = F(e + i * 4);
    }
    __imp__sub_000DCA10(ctx, base);
    if (!g_beam) return;
    float d[3];
    for (int i = 0; i < 3; ++i) d[i] = F(e + i * 4) - F(s + i * 4);
    float l = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    int hit = ctx.eax & 0xFF;
    BeamState& b = g_beams[self];
    if (hit != b.hit || std::fabs(l - b.len) > 0.5f) {
        LOGR("beam %08X frame %u ray hit %d len %.2f (asked %.2f, end moved %.2f %.2f %.2f) cast1 hit %u obj %08X recast %u hit %u obj %08X",
             self, Frame(), hit, l, len, F(e) - before[0], F(e + 4) - before[1], F(e + 8) - before[2], g_c1hit, g_c1obj,
             g_c2, g_c2hit, g_c2obj);
        b.hit = hit;
        b.len = l;
    }
}

X86_FUNC(sub_000DCD30) {
    uint32_t self = ctx.ecx, out = 0;
    if (g_beam) std::memcpy(&out, kernel::HostPtr(ctx.esp + 8), 4);
    __imp__sub_000DCD30(ctx, base);
    if (!g_beam) return;
    float f = F(out);
    BeamState& b = g_beams[self];
    if (std::fabs(f - b.fade) > 0.05f) {
        LOGR("beam %08X frame %u fade %.3f", self, Frame(), f);
        b.fade = f;
    }
}

// 0xDD060: the light beam's draw (thiscall, no args). Logs each beam's calls with the time since its last one, so
// frames where a beam is not drawn show as gaps, and what its early checks saw.
X86_EXTERN_FUNC(__imp__sub_000DD060);
#include <chrono>
namespace {
std::unordered_map<uint32_t, double> g_beam_last;
uint32_t g_draws = 0;
double Now() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}
uint32_t U(uint32_t a) { uint32_t v; std::memcpy(&v, kernel::HostPtr(a), 4); return v; }
}  // namespace

X86_FUNC(sub_000DD060) {
    uint32_t self = ctx.ecx;
    if (g_beam) {
        double t = Now();
        double& last = g_beam_last[self];
        double gap = last ? t - last : 0;
        last = t;
        uint32_t o = U(self + 0x0C);
        uint32_t en = o ? (U(o + U(0x434AA0)) & 0xFF) : 0xFFFF;
        uint32_t cnt = o ? U(o + U(0x434AB0)) : 0xFFFF;
        float scale = o ? F(o + U(0x434AAC)) : -1;
        if (gap > 25 || (++g_draws % 300) == 0)
            LOGR("beamdraw %08X gap %.1f ms, obj %08X enabled %u count %u scale %.3f", self, gap, o, en, cnt, scale);
    }
    __imp__sub_000DD060(ctx, base);
}

// the sphere test's center is drawable+0x04 when called from a room's list (0x194200); a light beam's drawable has
// the vtable 0x3150EC and its beam at +0x30
namespace { std::unordered_map<uint32_t, double> g_tested_last; }
bool BeamDrawableTested(uint32_t center, uint32_t culled) {
    if (!g_beam || center < 4) return false;
    uint32_t d = center - 4;
    if (U(d) != 0x3150EC) return false;
    double t = Now();
    double& last = g_tested_last[d];
    double gap = last ? t - last : 0;
    last = t;
    static uint32_t n = 0;
    if (gap > 25 || culled || (++n % 300) == 0)
        LOGR("beamtest drawable %08X beam %08X gap %.1f ms culled %u flags %08X radius %.1f", d, U(d + 0x30), gap,
             culled, U(d + 0x14), F(d + 0x10));
    return true;
}

// UNCHAINED_DBG_PORTALS=1: every portal test (0x1BC980, thiscall, 3 args) reports "all visible" (result byte 1)
X86_EXTERN_FUNC(__imp__sub_001BC980);
namespace { const bool g_portals = Env("UNCHAINED_DBG_PORTALS"); }
X86_FUNC(sub_001BC980) {
    __imp__sub_001BC980(ctx, base);
    if (g_portals && ctx.eax) *static_cast<uint8_t*>(kernel::HostPtr(ctx.eax)) = 1;
}
