"""Unchained's automated play tests: each scenario in scenarios.toml runs the game once, visibly, with its own settings
and scripted input (UNCHAINED_INPUT), lets the renderer capture its own frames at set times (XBE_CAPTURE_AT, never a
screen grab) and writes every present's time (XBE_FRAMETIMES). Then it checks the run and writes a report:

  crash        the game ended before the test closed it, or left a crash dump
  log          error lines in the game's log (the F3 thread dump excepted)
  picture      a capture that is blank or flat
  resolution   the render scale under what the window needs (render_percent 100 or more)
  reference    a capture much darker, brighter or greyer than the reference video at the same moment
  frame times  average fps, 1% and 0.1% lows, p50/p99/p99.9/max, hitches over 25/33/50/100 ms, stutter, pacing

  py -3 tools/tests/run_tests.py                 every scenario
  py -3 tools/tests/run_tests.py lab title       the ones named
  py -3 tools/tests/run_tests.py --list

The game files, the build and the reference video stay outside the repo; paths are options (--exe, --reference).
Exit status 1 when a check fails.
"""
import argparse
import csv
import ctypes
import datetime
import math
import os
import re
import shutil
import subprocess
import sys
import time
import tomllib
from ctypes import wintypes
from pathlib import Path

from PIL import Image, ImageDraw, ImageStat

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
DEFAULT_EXE = REPO / "out" / "build" / "release" / "Unchained.exe"
DEFAULT_REFERENCE = Path(r"C:\Unchained-private\reference\whiplash-reference-4k.mkv")
DEFAULT_OUT = Path(r"C:\Unchained-private\turns")

# checks' limits
BLANK_MEAN = 2.0        # mean luma (0-255) under this: a black picture (dark scenes sit near 5)
FLAT_STDDEV = 3.0       # luma spread under this: one colour
REF_DARK = 0.75         # our mean luma over the reference's under this: too dark
REF_BRIGHT = 1.35       # over this: too bright
REF_GREY = 0.6          # our mean saturation over the reference's under this: washed out
SCALE_SLACK = 0.98      # render scale at least this share of what the window needs
TARGET_MS = 1000.0 / 60
HITCH_MS = (25, 33, 50, 100)
STUTTER_MS = 4.0

user32 = ctypes.WinDLL("user32", use_last_error=True)
WM_CLOSE = 0x0010
SPI_GETWORKAREA = 0x0030
EnumWindowsProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def game_windows():
    """(hwnd, pid) of every window of class Unchained"""
    out = []

    def cb(hwnd, _):
        name = ctypes.create_unicode_buffer(64)
        user32.GetClassNameW(hwnd, name, 64)
        if name.value == "Unchained" and user32.IsWindowVisible(hwnd):
            pid = wintypes.DWORD()
            user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
            out.append((hwnd, pid.value))
        return True

    user32.EnumWindows(EnumWindowsProc(cb), 0)
    return out


def work_area():
    r = wintypes.RECT()
    user32.SystemParametersInfoW(SPI_GETWORKAREA, 0, ctypes.byref(r), 0)
    return r.left, r.top, r.right - r.left, r.bottom - r.top


def place(hwnd, rect):
    SWP_NOZORDER, SWP_NOACTIVATE = 0x0004, 0x0010
    user32.SetWindowPos(hwnd, None, rect[0], rect[1], rect[2], rect[3], SWP_NOZORDER | SWP_NOACTIVATE)


def unchained_pids():
    try:
        text = subprocess.run(["tasklist", "/FI", "IMAGENAME eq Unchained.exe", "/FO", "CSV", "/NH"],
                              capture_output=True, text=True).stdout
    except OSError:
        return set()
    return {int(row[1]) for row in csv.reader(text.splitlines()) if len(row) > 1 and row[1].isdigit()}


def settings_text(table):
    lines = []
    for k, v in table.items():
        if isinstance(v, bool):
            lines.append(f"{k} = {'true' if v else 'false'}")
        elif isinstance(v, str):
            lines.append(f'{k} = "{v}"')
        else:
            lines.append(f"{k} = {v}")
    return "\n".join(lines) + "\n"


def script_of(sc):
    """the scenario's input: input_file (a recorded run, tools/tests/runs/, read by the game) or the input line"""
    if "input_file" in sc:
        return "@" + str((HERE / sc["input_file"]).resolve())
    return sc.get("input", "")


def newest_log(exe_dir, since):
    logs = [p for p in (exe_dir / "logs").glob("unchained_*.log") if p.stat().st_mtime >= since - 1]
    return max(logs, key=lambda p: p.stat().st_mtime) if logs else None


def run_scenario(name, sc, base, exe, out):
    """runs the game once; returns the run's record"""
    exe_dir = exe.parent
    rec = {"name": name, "dir": out, "failures": [], "warnings": [], "captures": [], "notes": []}
    out.mkdir(parents=True, exist_ok=True)
    cap_dir = out / "cap"
    cap_dir.mkdir(exist_ok=True)
    frametimes = out / "frametimes.csv"

    settings_file = exe_dir / "settings.toml"
    backup = settings_file.read_bytes() if settings_file.exists() else None
    table = dict(base)
    table.update(sc.get("settings", {}))
    settings_file.write_text(settings_text(table))

    at = sc.get("capture_at", [])
    env = dict(os.environ)
    env.pop("UNCHAINED_OFFSCREEN", None)
    env.update(sc.get("env", {}))
    env.update({"XBE_CAPTURE_DIR": str(cap_dir), "XBE_CAPTURE_AT": ",".join(str(t) for t in at),
                "XBE_FRAMETIMES": str(frametimes), "UNCHAINED_INPUT": script_of(sc),
                "XBE_AUTOSTART": "1"})  # past the "game is ready" prompt after a code build
    dumps = exe_dir / "CrashDumps"
    dumps_before = set(dumps.glob("*.dmp")) if dumps.exists() else set()
    before = unchained_pids()
    start = time.time()
    proc = subprocess.Popen([str(exe)], cwd=str(exe_dir), env=env)
    run_for = sc.get("duration", (max(at) if at else 30) + 5)
    rect = sc.get("window")
    rect = work_area() if not rect else (40, 40, rect[0], rect[1])
    placed = set()
    shown_at = None
    ended_early = False
    try:
        # the game may build its code on a first start and relaunch itself: follow whichever copy has the window
        deadline = start + run_for + sc.get("build_wait", 240)
        while time.time() < deadline:
            wins = [(h, p) for h, p in game_windows() if p not in before]
            for h, p in wins:
                # again whenever it moved: the game applies its own window size from the settings once it is up
                r = wintypes.RECT()
                user32.GetWindowRect(h, ctypes.byref(r))
                if (r.left, r.top, r.right - r.left, r.bottom - r.top) != tuple(rect):
                    place(h, rect)
                placed.add(h)
                shown_at = shown_at or time.time()
            pids = unchained_pids() - before
            if not pids and time.time() - start > 5:
                ended_early = True
                break
            if shown_at and time.time() - shown_at >= run_for:
                break
            time.sleep(0.2)
        for h, p in game_windows():
            if p not in before:
                user32.PostMessageW(h, WM_CLOSE, 0, 0)
        for _ in range(40):
            if not (unchained_pids() - before):
                break
            time.sleep(0.2)
        for pid in unchained_pids() - before:
            subprocess.run(["taskkill", "/F", "/PID", str(pid)], capture_output=True)
    finally:
        if backup is None:
            settings_file.unlink(missing_ok=True)
        else:
            settings_file.write_bytes(backup)

    if ended_early:
        rec["failures"].append("crash: the game closed before the test ended")
    new_dumps = (set(dumps.glob("*.dmp")) if dumps.exists() else set()) - dumps_before
    if new_dumps:
        rec["failures"].append(f"crash: dump {', '.join(p.name for p in new_dumps)}")

    log = newest_log(exe_dir, start)
    rec["log"] = log
    if log:
        shutil.copy(log, out / "game.log")
        check_log(log, rec, table, rect)
    else:
        rec["failures"].append("log: none written")

    # captures in time order: each XBE_CAPTURE_AT time made a folder named by the clock
    folders = sorted(p for p in cap_dir.iterdir() if p.is_dir())
    for t, folder in zip(at, folders):
        shown = folder / "shown.bmp"
        if not shown.exists():
            rec["failures"].append(f"picture: no capture at {t} s")
            continue
        png = out / f"t{int(t):03d}.png"
        img = Image.open(shown).convert("RGB")
        img.save(png)
        rec["captures"].append({"t": t, "png": png, "folder": folder, "img": img})
        check_picture(img, t, rec)
    if len(folders) < len(at):
        rec["failures"].append(f"picture: {len(folders)} of {len(at)} captures")
    rec["frames"] = frame_stats(frametimes, sc.get("perf_from", 0), at)
    return rec


def check_log(log, rec, settings, rect):
    errors = []
    for line in log.read_text(errors="replace").splitlines():
        if "[error]" not in line and "[fatal]" not in line:
            continue
        body = line.split("]", 3)[-1].strip()
        # F3's thread dump: registers and stack words of the game thread
        if re.match(r"^(eax|\[esp\+)", body):
            continue
        errors.append(body)
    if errors:
        rec["warnings"].append(f"log: {len(errors)} error lines, first: {errors[0][:160]}")
    m = re.findall(r"(\d+)x(\d+) guest, (\d+)x(\d+) window, render scale ([\d.]+)", log.read_text(errors="replace"))
    if m:
        gw, gh, ww, wh, scale = m[-1]
        rec["notes"].append(f"render: {gw}x{gh} guest, {ww}x{wh} window, scale {scale}")
        pct = settings.get("render_percent", 100)
        # the 448 lines the game shows, at least as tall as the window (its width covered when widened)
        need = int(wh) / 448
        if pct >= 100 and float(scale) < need * SCALE_SLACK:
            rec["failures"].append(f"resolution: render scale {scale} under the {need:.3f} the window needs")


def luma_sat(img):
    small = img.resize((320, int(320 * img.height / img.width)))
    luma = ImageStat.Stat(small.convert("L"))
    sat = ImageStat.Stat(small.convert("HSV").split()[1])
    return luma.mean[0], luma.stddev[0], sat.mean[0]


def check_picture(img, t, rec):
    mean, dev, _ = luma_sat(img)
    if mean < BLANK_MEAN:
        rec["failures"].append(f"picture: black at {t} s (mean luma {mean:.1f})")
    elif dev < FLAT_STDDEV:
        rec["failures"].append(f"picture: flat at {t} s (luma spread {dev:.1f})")


def reference_frame(video, seconds, cache):
    cache.mkdir(parents=True, exist_ok=True)
    png = cache / f"ref-{seconds:07.2f}.png"
    if not png.exists():
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-ss", str(seconds), "-i", str(video), "-frames:v", "1",
                        "-vf", "scale=1280:-1", str(png)], check=False)
    return Image.open(png).convert("RGB") if png.exists() else None


def compare_reference(rec, sc, video, cache):
    refs = sc.get("reference_at", [])
    for cap, ref_t in zip(rec["captures"], refs):
        if ref_t is None:
            continue
        ref = reference_frame(video, ref_t, cache)
        if ref is None:
            rec["warnings"].append(f"reference: no frame at {ref_t} s")
            continue
        cap["ref"] = ref
        cap["ref_t"] = ref_t
        ours_l, _, ours_s = luma_sat(cap["img"])
        ref_l, _, ref_s = luma_sat(ref)
        lr = ours_l / max(ref_l, 1)
        sr = ours_s / max(ref_s, 1)
        cap["cmp"] = f"luma {ours_l:.0f} vs {ref_l:.0f} ({lr:.2f}x), saturation {ours_s:.0f} vs {ref_s:.0f} ({sr:.2f}x)"
        if lr < REF_DARK:
            rec["warnings"].append(f"reference: {lr:.2f}x the reference's brightness at {cap['t']} s (ref {ref_t} s)")
        elif lr > REF_BRIGHT:
            rec["warnings"].append(f"reference: {lr:.2f}x the reference's brightness at {cap['t']} s (ref {ref_t} s)")
        if sr < REF_GREY:
            rec["warnings"].append(f"reference: {sr:.2f}x the reference's saturation at {cap['t']} s")


def percentile(sorted_vals, p):
    if not sorted_vals:
        return 0.0
    k = (len(sorted_vals) - 1) * p / 100
    lo, hi = math.floor(k), math.ceil(k)
    return sorted_vals[lo] + (sorted_vals[hi] - sorted_vals[lo]) * (k - lo)


CAPTURE_SKIP_S = 1.0  # a capture's read-backs stall the frames after it: those are the test's, not the game's


def frame_stats(path, from_s, captures=()):
    if not path.exists():
        return None
    rows = []
    with open(path) as f:
        for r in csv.DictReader(f):
            try:
                rows.append((float(r["time_ms"]), float(r["interval_ms"])))
            except (KeyError, ValueError):
                pass
    def near_capture(t):
        return any(c * 1000 <= t <= (c + CAPTURE_SKIP_S) * 1000 for c in captures)

    dts = [dt for t, dt in rows[1:] if t >= from_s * 1000 and not near_capture(t)]
    if len(dts) < 10:
        return None
    s = sorted(dts)
    n = len(dts)

    def low(share):  # the fps of the slowest share of frames
        k = max(1, int(n * share))
        return 1000.0 / (sum(s[-k:]) / k)

    worst = sorted(((dt, i) for i, dt in enumerate(dts)), reverse=True)[:5]
    return {
        "frames": n, "avg_fps": 1000.0 * n / sum(dts), "low1": low(0.01), "low01": low(0.001),
        "p50": percentile(s, 50), "p99": percentile(s, 99), "p999": percentile(s, 99.9), "max": s[-1],
        "hitches": {h: sum(1 for d in dts if d > h) for h in HITCH_MS},
        "stutter": sum(1 for a, b in zip(dts, dts[1:]) if abs(b - a) > STUTTER_MS),
        "pacing": sum(1 for d in dts if abs(d - TARGET_MS) <= 2.0) / n,
        "worst": [(round(dt, 1), i) for dt, i in worst],
    }


def sheet(records, path):
    tw, th, label = 640, 360, 22
    rows = [(r, c) for r in records for c in r["captures"]]
    if not rows:
        return
    img = Image.new("RGB", (tw * 2, len(rows) * (th + label)), (40, 40, 40))
    d = ImageDraw.Draw(img)
    for i, (r, c) in enumerate(rows):
        y = i * (th + label)
        d.text((6, y + 4), f"{r['name']} @ {c['t']} s", fill=(255, 255, 255))
        img.paste(c["img"].resize((tw, th)), (0, y + label))
        if "ref" in c:
            d.text((tw + 6, y + 4), f"reference @ {c['ref_t']} s  {c.get('cmp', '')}", fill=(255, 255, 255))
            img.paste(c["ref"].resize((tw, th)), (tw, y + label))
    img.save(path)


def report(records, path):
    lines = [f"# Unchained tests {datetime.datetime.now():%Y-%m-%d %H:%M}", ""]
    for r in records:
        status = "FAIL" if r["failures"] else ("warn" if r["warnings"] else "pass")
        lines += [f"## {r['name']}: {status}", ""]
        lines += [f"- FAIL {m}" for m in r["failures"]]
        lines += [f"- warn {m}" for m in r["warnings"]]
        lines += [f"- {m}" for m in r["notes"]]
        for c in r["captures"]:
            lines.append(f"- capture {c['t']} s: `{c['png'].name}`" + (f", {c['cmp']}" if "cmp" in c else ""))
        f = r.get("frames")
        if f:
            h = ", ".join(f">{k} ms {v}" for k, v in f["hitches"].items())
            lines += [f"- frames {f['frames']}: {f['avg_fps']:.1f} fps avg, 1% low {f['low1']:.1f}, 0.1% low "
                      f"{f['low01']:.1f}; p50 {f['p50']:.2f} p99 {f['p99']:.2f} p99.9 {f['p999']:.2f} max "
                      f"{f['max']:.1f} ms; hitches {h}; stutter {f['stutter']}; on pace {f['pacing'] * 100:.0f}%; "
                      f"worst (ms, frame) {f['worst']}"]
        lines.append("")
    path.write_text("\n".join(lines))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("names", nargs="*")
    ap.add_argument("--exe", type=Path, default=DEFAULT_EXE)
    ap.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    ap.add_argument("--out", type=Path, default=DEFAULT_OUT)
    ap.add_argument("--list", action="store_true")
    a = ap.parse_args()
    cfg = tomllib.loads((HERE / "scenarios.toml").read_text())
    scenarios = cfg["scenario"]
    if a.list:
        for n, sc in scenarios.items():
            print(f"{n:16} {sc.get('about', '')}")
        return 0
    names = a.names or list(scenarios)
    unknown = [n for n in names if n not in scenarios]
    if unknown:
        print("unknown:", ", ".join(unknown))
        return 2
    if unchained_pids():
        print("Unchained is running: close it first (the tests own the settings file while they run)")
        return 2
    out = a.out / f"tests-{datetime.datetime.now():%Y%m%d-%H%M%S}"
    records = []
    for n in names:
        print(f"[{n}] running", flush=True)
        rec = run_scenario(n, scenarios[n], cfg.get("base_settings", {}), a.exe, out / n)
        if a.reference.exists():
            compare_reference(rec, scenarios[n], a.reference, a.out / "reference-cache")
        records.append(rec)
        print(f"[{n}] {'FAIL' if rec['failures'] else 'ok'}: " + "; ".join(rec["failures"] + rec["warnings"]),
              flush=True)
    report(records, out / "report.md")
    sheet(records, out / "sheet.png")
    print(f"report {out / 'report.md'}\nsheet  {out / 'sheet.png'}")
    return 1 if any(r["failures"] for r in records) else 0


if __name__ == "__main__":
    sys.exit(main())
