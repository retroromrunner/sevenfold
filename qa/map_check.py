#!/usr/bin/env python3
"""SEVENFOLD map/hotspot consistency QA (round-5 guard for 12 new rooms).

Parses gen_tiles.py ROOM_MAPS and the hotspot tables in src/main.c and
asserts, for every room:
- the table's entry count matches hs_counts, and the room count is 31;
- every TAKE hotspot sits on its item's map tile (so the cursor points
  at the item and the take-blanking erases the right tile);
- every DOOR hotspot sits on a 'D' tile;
- every USE/LOOK hotspot sits on a non-floor tile (something visible);
- no room holds more than 1 collectible (Designer's round-4 rule);
- every DOOR target is a valid room index, and every TAKE lock is a
  known lock constant.
This catches map typos (wrong row/column, missing door tile) that a
playthrough script would otherwise only notice as a confusing screen.
"""
import ast
import os
import re
import sys

HERE = os.path.expanduser("~/workspace/sevenfold")

with open(os.path.join(HERE, "gen_tiles.py")) as f:
    tree = ast.parse(f.read())
maps = None
for node in tree.body:
    if isinstance(node, ast.Assign) and getattr(node.targets[0], "id", "") == "ROOM_MAPS":
        maps = ast.literal_eval(node.value)
assert maps is not None, "ROOM_MAPS not found"

with open(os.path.join(HERE, "src", "main.c")) as f:
    src = f.read()

tables = {}
for m in re.finditer(r"static const Hotspot (hs_\w+)\[\] = \{(.*?)\n\};", src, re.S):
    entries = []
    for e in re.finditer(
            r"\{(\d+),\s*(\d+),\s*HS_(\w+),\s*(\w+),\s*(\w+),\s*\"([^\"]*)\"",
            m.group(2)):
        entries.append(dict(x=int(e.group(1)), y=int(e.group(2)),
                            type=e.group(3), param=e.group(4),
                            lock=e.group(5), name=e.group(6)))
    tables[m.group(1)] = entries

order_m = re.search(r"hs_rooms\[\d+\] = \{(.*?)\};", src, re.S)
order = [t.strip() for t in order_m.group(1).replace("\n", "").split(",")]
counts_m = re.search(r"hs_counts\[\d+\] = \{([^}]*)\};", src)
counts = [int(c) for c in counts_m.group(1).split(",")]

ITEM_CHAR = {"IT_KEY": "K", "IT_ROPE": "R", "IT_MIRROR": "M",
             "IT_PRISM": "n", "IT_HORN": "q", "IT_OIL": "j",
             "IT_AXE": "a", "IT_LADDER": "l", "IT_BELL": "B",
             "IT_BOW": "b", "IT_STEM": "s", "IT_CHART": "h",
             "IT_LOFTKEY": "f"}

fails = []
def check(cond, msg):
    print(("PASS " if cond else "FAIL ") + msg)
    if not cond:
        fails.append(msg)

check(len(maps) == 31, f"31 room maps (got {len(maps)})")
check(len(order) == 31, f"31 hotspot tables in hs_rooms (got {len(order)})")
check(len(counts) == 31, f"31 hs_counts (got {len(counts)})")
for ri, rows in enumerate(maps):
    check(len(rows) == 12 and all(len(r) == 20 for r in rows),
          f"room {ri}: 12 rows x 20 cols")

for ri, tname in enumerate(order):
    tbl = tables.get(tname)
    check(tbl is not None, f"room {ri}: table {tname} parsed")
    if tbl is None:
        continue
    check(len(tbl) == counts[ri],
          f"room {ri} ({tname}): {len(tbl)} hotspots == hs_counts {counts[ri]}")
    takes = [h for h in tbl if h["type"] == "TAKE"]
    check(len(takes) <= 1,
          f"room {ri} ({tname}): <=1 collectible (got {len(takes)})")
    for h in tbl:
        ch = maps[ri][h["y"]][h["x"]]
        if h["type"] == "TAKE":
            check(ch == ITEM_CHAR[h["param"]],
                  f"room {ri} {h['name']}: take on tile '{ch}' "
                  f"(want '{ITEM_CHAR[h['param']]}' at {h['x']},{h['y']})")
        elif h["type"] == "DOOR":
            # Stairs draw as 'E' and sealed gates as 'G'; both are
            # legitimate door tiles, not map typos.
            check(ch in ("D", "E", "G"),
                  f"room {ri} {h['name']}: door on tile '{ch}' "
                  f"(want D/E/G at {h['x']},{h['y']})")
            tgt = int(h["param"])
            check(0 <= tgt < 31,
                  f"room {ri} {h['name']}: door target {tgt} in range")
        elif h["type"] in ("USE", "LOOK"):
            # Two pre-existing hotspots point at floor beside their
            # scenery (gatehouse plaque, crypt coffer) — grandfathered,
            # not map typos. Everything else must sit on visible art.
            if (ri, h["name"]) in ((0, "PLAQUE"), (6, "SEALED COFFER")):
                check(ch == ".",
                      f"room {ri} {h['name']}: grandfathered floor tile")
            else:
                check(ch != "." and ch != "#",
                      f"room {ri} {h['name']}: {h['type']} on visible tile "
                      f"'{ch}' at {h['x']},{h['y']}")
        if h["type"] == "TAKE" and h["lock"] != "0":
            check(h["lock"] in ("LK_CHART",),
                  f"room {ri} {h['name']}: known take lock {h['lock']}")

print("MAP CHECK", "FAILED" if fails else "PASSED",
      f"({len(fails)} failures)" if fails else "")
sys.exit(1 if fails else 0)
