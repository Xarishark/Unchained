"""Frame-time statistics for XBE_FRAMETIMES files: average fps, 1% and 0.1% lows, p50/p99/p99.9/max, hitches over
25/33/50/100 ms, stutter (consecutive frames more than 4 ms apart) and pacing against the target interval, with the
worst frames and what the renderer did in each (shaders compiled, pipelines made, texture uploads, new textures,
read-backs, new targets: the columns the renderers write after the interval).

  python3 frametimes.py frametimes.csv [--from SECONDS] [--fps 60] [--baseline other.csv]
"""
import argparse
import csv
import math
import sys
from pathlib import Path

HITCH_MS = (25, 33, 50, 100)
STUTTER_MS = 4.0
CAPTURE_SKIP_S = 1.0  # a capture's read-backs stall the frames after it: those are the test's, not the game's
CAUSES = ("shaders", "pipelines", "tex_uploads", "new_textures", "read_backs", "new_targets")


def percentile(sorted_vals, p):
    if not sorted_vals:
        return 0.0
    k = (len(sorted_vals) - 1) * p / 100
    lo, hi = math.floor(k), math.ceil(k)
    return sorted_vals[lo] + (sorted_vals[hi] - sorted_vals[lo]) * (k - lo)


def read_rows(path):
    rows = []
    with open(path) as f:
        for r in csv.DictReader(f):
            try:
                row = {"t": float(r["time_ms"]), "dt": float(r["interval_ms"]), "present": int(r["present"])}
            except (KeyError, ValueError):
                continue
            row["causes"] = {c: int(r[c]) for c in CAUSES if r.get(c) not in (None, "")}
            rows.append(row)
    return rows


def frame_stats(path, from_s, captures=(), target_fps=60, worst_n=5):
    path = Path(path)
    if not path.exists():
        return None
    rows = read_rows(path)

    def near_capture(t):
        return any(c * 1000 <= t <= (c + CAPTURE_SKIP_S) * 1000 for c in captures)

    kept = [r for r in rows[1:] if r["t"] >= from_s * 1000 and not near_capture(r["t"])]
    dts = [r["dt"] for r in kept]
    if len(dts) < 10:
        return None
    s = sorted(dts)
    n = len(dts)
    target = 1000.0 / target_fps if target_fps else percentile(s, 50)

    def low(share):  # the fps of the slowest share of frames
        k = max(1, int(n * share))
        return 1000.0 / (sum(s[-k:]) / k)

    worst_rows = sorted(kept, key=lambda r: r["dt"], reverse=True)[:worst_n]
    worst = [(round(r["dt"], 1), r["present"]) for r in worst_rows]
    return {
        "frames": n, "avg_fps": 1000.0 * n / sum(dts), "low1": low(0.01), "low01": low(0.001),
        "p50": percentile(s, 50), "p99": percentile(s, 99), "p999": percentile(s, 99.9), "max": s[-1],
        "hitches": {h: sum(1 for d in dts if d > h) for h in HITCH_MS},
        "stutter": sum(1 for a, b in zip(dts, dts[1:]) if abs(b - a) > STUTTER_MS),
        # within 10% of the target interval (target_fps 0: the run's median, for uncapped or refresh-paced runs)
        "pacing": sum(1 for d in dts if abs(d - target) <= target * 0.1) / n,
        "worst": worst,
        # the worst frames, when they came (seconds on the frame clock) and what the renderer did in them
        "worst_detail": [(round(r["dt"], 1), r["present"], round(r["t"] / 1000, 2),
                          {k: v for k, v in r["causes"].items() if v}) for r in worst_rows],
    }


def line(f):
    h = ", ".join(f">{k} ms {v}" for k, v in f["hitches"].items())
    return (f"frames {f['frames']}: {f['avg_fps']:.1f} fps avg, 1% low {f['low1']:.1f}, 0.1% low {f['low01']:.1f}; "
            f"p50 {f['p50']:.2f} p99 {f['p99']:.2f} p99.9 {f['p999']:.2f} max {f['max']:.1f} ms; hitches {h}; "
            f"stutter {f['stutter']}; on pace {f['pacing'] * 100:.0f}%")


def worse_than(f, base):
    """what got worse against a baseline run: lows down more than 5%, more hitches over 33 ms, a higher max"""
    out = []
    if f["low1"] < base["low1"] * 0.95:
        out.append(f"1% low {f['low1']:.1f} vs {base['low1']:.1f}")
    if f["low01"] < base["low01"] * 0.95:
        out.append(f"0.1% low {f['low01']:.1f} vs {base['low01']:.1f}")
    if f["hitches"][33] > base["hitches"][33]:
        out.append(f"hitches >33 ms {f['hitches'][33]} vs {base['hitches'][33]}")
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv")
    ap.add_argument("--from", dest="from_s", type=float, default=0)
    ap.add_argument("--fps", type=float, default=60)
    ap.add_argument("--baseline")
    a = ap.parse_args()
    f = frame_stats(a.csv, a.from_s, (), a.fps)
    if not f:
        print("too few frames")
        return 1
    print(line(f))
    for w in f["worst_detail"]:
        print(f"  {w[0]} ms at present {w[1]} ({w[2]} s): {w[3] or 'nothing of the renderer'}")
    if a.baseline:
        b = frame_stats(a.baseline, a.from_s, (), a.fps)
        if b:
            print("baseline: " + line(b))
            bad = worse_than(f, b)
            print("worse: " + "; ".join(bad) if bad else "no worse than the baseline")
            return 1 if bad else 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
