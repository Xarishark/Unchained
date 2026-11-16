"""The PC port's progress from docs/TODO.md as a table with bars, written between the PROGRESS markers in
docs/STATUS.md. Each item counts by its weight (a section's items weigh the same within it): [x] done counts whole,
[~] started half, [ ] none.

  py -3 tools/todo_progress.py
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TODO = ROOT / "docs" / "TODO.md"
STATUS = ROOT / "docs" / "STATUS.md"
BEGIN, END = "<!-- PROGRESS -->", "<!-- /PROGRESS -->"
WIDTH = 20  # characters in a bar
SKIP = {"Not applicable"}

sections = []
current = None
for line in TODO.read_text(encoding="utf-8").splitlines():
    head = re.match(r"^#{2,3} (.+)", line)
    if head:
        current = {"name": head.group(1).strip(), "done": 0.0, "count": 0}
        sections.append(current)
        continue
    item = re.match(r"^- \[([ x~])\]", line)
    if item and current and current["name"] not in SKIP:
        current["count"] += 1
        current["done"] += {"x": 1.0, "~": 0.5, " ": 0.0}[item.group(1)]

rows = ["| Area | Progress | Done |", "|---|---|---|"]
total_done = total = 0.0
for s in sections:
    if not s["count"]:
        continue
    share = s["done"] / s["count"]
    total_done += s["done"]
    total += s["count"]
    bar = "█" * round(share * WIDTH) + "░" * (WIDTH - round(share * WIDTH))
    rows.append(f"| {s['name']} | `{bar}` | {share * 100:.0f}% ({s['done']:g} of {s['count']}) |")
share = total_done / total if total else 0
bar = "█" * round(share * WIDTH) + "░" * (WIDTH - round(share * WIDTH))
rows.append(f"| **All** | `{bar}` | **{share * 100:.0f}%** ({total_done:g} of {total:g}) |")
block = "\n".join([BEGIN, "PC port features from [TODO.md](TODO.md) (started items count half):", "", *rows, END])

text = STATUS.read_text(encoding="utf-8")
if BEGIN in text:
    text = text[: text.index(BEGIN)] + block + text[text.index(END) + len(END):]
else:
    first = text.index("\n") + 1  # after the title
    text = text[:first] + "\n" + block + "\n" + text[first:]
STATUS.write_text(text, encoding="utf-8", newline="\n")
print(f"progress: {share * 100:.0f}% of {total:g} items, written to {STATUS.name}")
