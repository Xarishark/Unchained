"""Frame-time suite on Linux or Windows: scenarios from scenarios.toml, each run in a data folder of its own
(UNCHAINED_DATA_ROOT) with the game's files linked in, the scenario's scripted input, the renderer's own captures at
set times and every present's time. cold scenarios start without the shader and pipeline caches. Each run's report
has the statistics of frametimes.py and the worst frames with what the renderer did in them; --repeat runs each
scenario again, --baseline compares with an earlier report's numbers.

  python3 tools/tests/run_suite.py --exe out/build/linux-release/Unchained --game ~/game/assets --out ~/runs/vk title lobby
  py -3 tools/tests/run_suite.py --exe out\\build\\release\\Unchained.exe --game C:\\game\\assets --out C:\\runs\\d3d12 title

The game window opens at the scenario's settings (windowed 640x480 unless it says otherwise); the run ends with the
duration and the game is asked to close (SIGTERM on Linux, WM_CLOSE on Windows).
"""
import argparse
import json
import os
import shutil
import signal
import subprocess
import sys
import time
import tomllib
from pathlib import Path

import frametimes

HERE = Path(__file__).resolve().parent
WINDOWS = os.name == "nt"


def settings_text(table):
    out = []
    for k, v in table.items():
        if isinstance(v, bool):
            out.append(f"{k} = {'true' if v else 'false'}")
        elif isinstance(v, (int, float)):
            out.append(f"{k} = {v}")
        else:
            out.append(f'{k} = "{v}"')
    return "\n".join(out) + "\n"


def link_game(data, game):
    """the game's files at data/assets: a symbolic link, a junction on Windows"""
    target = data / "assets"
    if target.exists():
        return
    if WINDOWS:
        subprocess.run(["cmd", "/c", "mklink", "/J", str(target), str(game)], check=True, capture_output=True)
    else:
        target.symlink_to(game)


def script_of(sc):
    if "input_file" in sc:
        return "@" + str((HERE / sc["input_file"]).resolve())
    return sc.get("input", "")


def run(name, sc, base, exe, game, out, data, cold, renderer, hdd):
    out.mkdir(parents=True, exist_ok=True)
    data.mkdir(parents=True, exist_ok=True)
    link_game(data, game)
    # the scenarios' recorded input expects a profile on the console's disk: a copy of one, fresh each run
    if hdd:
        shutil.rmtree(data / "hdd", ignore_errors=True)
        shutil.copytree(hdd, data / "hdd")
    if cold:
        shutil.rmtree(data / "cache", ignore_errors=True)
    table = dict(base)
    table.update(sc.get("settings", {}))
    if renderer:
        table["renderer"] = renderer
    (data / "settings.toml").write_text(settings_text(table))
    at = sc.get("capture_at", [])
    cap_dir = out / "cap"
    shutil.rmtree(cap_dir, ignore_errors=True)
    cap_dir.mkdir()
    csv_path = out / "frametimes.csv"
    csv_path.unlink(missing_ok=True)
    env = dict(os.environ)
    env.update(sc.get("env", {}))
    env.update({"UNCHAINED_DATA_ROOT": str(data), "XBE_CAPTURE_DIR": str(cap_dir),
                "XBE_CAPTURE_AT": ",".join(str(t) for t in at), "XBE_FRAMETIMES": str(csv_path),
                "UNCHAINED_INPUT": script_of(sc), "XBE_AUTOSTART": "1"})
    duration = sc.get("duration", (max(at) if at else 30) + 5)
    start = time.time()
    proc = subprocess.Popen([str(exe)], cwd=str(exe.parent), env=env, stdin=subprocess.DEVNULL,
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    ended_early = False
    while time.time() - start < duration:
        if proc.poll() is not None:
            ended_early = True
            break
        time.sleep(0.2)
    if proc.poll() is None:
        if WINDOWS:
            subprocess.run(["taskkill", "/PID", str(proc.pid)], capture_output=True)
        else:
            proc.send_signal(signal.SIGTERM)
        try:
            proc.wait(15)
        except subprocess.TimeoutExpired:
            proc.kill()
    rec = {"name": name, "cold": cold, "ended_early": ended_early, "exit": proc.returncode}
    logs = sorted((data / "logs").glob("unchained*.log"), key=lambda p: p.stat().st_mtime)
    if logs:
        shutil.copy(logs[-1], out / "game.log")
        text = logs[-1].read_text(errors="replace")
        rec["errors"] = [l for l in text.splitlines() if " error" in l.lower() or "failed" in l.lower()][:20]
    folders = sorted(p for p in cap_dir.iterdir() if p.is_dir())
    rec["captures"] = [str(f / "shown.bmp") for f in folders if (f / "shown.bmp").exists()]
    rec["frames"] = frametimes.frame_stats(csv_path, sc.get("perf_from", 0), at, sc.get("target_fps", 60))
    return rec


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("names", nargs="*")
    ap.add_argument("--exe", required=True, type=Path)
    ap.add_argument("--game", required=True, type=Path, help="the folder with default.xbe and the game's files")
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("--repeat", type=int, default=2)
    ap.add_argument("--cold", action="store_true", help="every run without the shader and pipeline caches")
    ap.add_argument("--renderer", help="settings.toml renderer for the runs (Windows: d3d12 or vulkan)")
    ap.add_argument("--hdd", type=Path, help="a console disk (hdd folder) with a profile, copied into each run")
    ap.add_argument("--env", action="append", default=[], metavar="NAME=VALUE",
                    help="an environment variable for every run (repeatable), e.g. XBE_PERF=1")
    ap.add_argument("--set", action="append", default=[], metavar="KEY=VALUE",
                    help="a setting over the scenarios' (repeatable): display_mode=fullscreen width=0 height=0 is the "
                         "desktop's full resolution")
    ap.add_argument("--baseline", type=Path, help="an earlier run's report.json to compare with")
    a = ap.parse_args()
    cfg = tomllib.loads((HERE / "scenarios.toml").read_text())
    base = dict(cfg.get("base_settings", {}))
    extra_env = dict(kv.partition("=")[::2] for kv in a.env)
    for kv in a.set:
        k, _, v = kv.partition("=")
        base[k] = int(v) if v.lstrip("-").isdigit() else {"true": True, "false": False}.get(v, v)
    names = a.names or list(cfg["scenario"])
    records = []
    for name in names:
        sc = cfg["scenario"][name]
        # one data folder per scenario across its runs: the first starts without caches, the later ones warm, as a
        # player's second start is (--cold, or a cold scenario, clears them before every run)
        data = a.out / f"{name}-data"
        shutil.rmtree(data / "cache", ignore_errors=True)
        for i in range(a.repeat):
            out = a.out / f"{name}-{i + 1}"
            cold = a.cold or sc.get("cold", False) or i == 0
            sc_run = dict(sc)
            sc_run["env"] = {**sc.get("env", {}), **extra_env}
            rec = run(name, sc_run, base, a.exe.resolve(), a.game.resolve(), out, data,
                      a.cold or sc.get("cold", False), a.renderer, a.hdd.resolve() if a.hdd else None)
            rec["cold"] = cold
            records.append(rec)
            f = rec["frames"]
            print(f"{name} run {i + 1}{' (cold)' if cold else ''}: " + (frametimes.line(f) if f else "no frame times"))
            if f:
                for w in f["worst_detail"]:
                    print(f"    {w[0]} ms at present {w[1]} ({w[2]} s): {w[3] or 'nothing of the renderer'}")
            if rec["ended_early"]:
                print(f"    ended early, exit {rec['exit']}")
            for e in rec.get("errors", [])[:5]:
                print(f"    log: {e}")
    (a.out / "report.json").write_text(json.dumps(records, indent=1, default=str))
    status = 0
    if a.baseline and a.baseline.exists():
        base_recs = json.loads(a.baseline.read_text())
        for rec in records:
            match = [b for b in base_recs if b["name"] == rec["name"] and b.get("frames")]
            if rec["frames"] and match:
                bad = frametimes.worse_than(rec["frames"], match[0]["frames"])
                print(f"{rec['name']} against the baseline: " + ("; ".join(bad) if bad else "no worse"))
                status |= bool(bad)
    return status


if __name__ == "__main__":
    sys.exit(main())
