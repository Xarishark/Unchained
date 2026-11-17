"""Two run_suite.py outputs compared screen by screen: for every scenario run and capture time, the reference's
picture (say Windows D3D12) beside the candidate's (Vulkan) and a map of where they differ, with the share of pixels
that differ by more than a small step. The screens are what to look at; a high share is a lead, not a verdict (the
game's own animation moves between runs).

  py -3 tools/tests/compare_runs.py C:/runs/win-d3d12 C:/runs/linux-vk --out C:/runs/cmp
"""
import argparse
import sys
from pathlib import Path

from PIL import Image, ImageChops

STEP = 24  # a channel difference past this counts


def shots(run_dir):
    cap = run_dir / "cap"
    if not cap.exists():
        return []
    return [f / "shown.bmp" for f in sorted(p for p in cap.iterdir() if p.is_dir()) if (f / "shown.bmp").exists()]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("reference", type=Path)
    ap.add_argument("candidate", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    rows = []
    for run in sorted(p for p in a.reference.iterdir() if p.is_dir()):
        other = a.candidate / run.name
        for i, (r, c) in enumerate(zip(shots(run), shots(other))):
            ri = Image.open(r).convert("RGB")
            ci = Image.open(c).convert("RGB")
            if ci.size != ri.size:
                ci = ci.resize(ri.size)
            d = ImageChops.difference(ri, ci)
            px = d.get_flattened_data() if hasattr(d, "get_flattened_data") else d.getdata()
            m = [max(p) for p in px]
            share = sum(1 for v in m if v > STEP) / len(m)
            heat = d.point(lambda v: min(255, v * 4))
            sheet = Image.new("RGB", (ri.width * 3, ri.height))
            sheet.paste(ri, (0, 0))
            sheet.paste(ci, (ri.width, 0))
            sheet.paste(heat, (ri.width * 2, 0))
            name = f"{run.name}_{i}.png"
            sheet.save(a.out / name)
            rows.append((share, name))
    rows.sort(reverse=True)
    lines = [f"{share * 100:5.1f}% of pixels differ  {name}" for share, name in rows]
    (a.out / "summary.txt").write_text("\n".join(lines) + "\n")
    print("\n".join(lines))
    return 0


if __name__ == "__main__":
    sys.exit(main())
