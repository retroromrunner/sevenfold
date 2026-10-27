#!/usr/bin/env python3
"""SEVENFOLD bridge-graphic QA (round 5): after USEing wood at the
chasm, a log must visibly lie across the gap — not just text.

Poke-driven (no full route needed): give the axe, USE the courtyard
tree for wood, jump to the chasm, and read the VRAM tilemap (0x9800)
row 5 before and after the wood USE. Before: the gap row is T_DARK.
After: columns 1-18 of row 5 are T_BRIDGE. Then leave and re-enter
the chasm (via the observatory, with the lens flag poked) to prove the
bridge persists through the wipe redraw. Tile indices from
src/tiles.h: T_DARK=35, T_BRIDGE=49.
"""
import os
import sys

sys.path.insert(0, os.path.expanduser(
    "~/workspace/gbc/tools/venv/lib/python3.12/site-packages"))
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
from pyboy import PyBoy

ROM = os.path.expanduser("~/workspace/sevenfold/releases/sevenfold.gb")
OUT = os.path.expanduser("~/workspace/sevenfold/qa")
ST_TITLE, ST_ROOM, ST_TEXT, ST_INV = 0, 1, 2, 4
A = dict(state=0xC0B1, room=0xC0B2, hot=0xC0B3,
         items_lo=0xC0B7, items_hi=0xC0B8,
         lens=0xC0C0, bridge=0xC0C2)
T_DARK, T_BRIDGE = 35, 49

pb = PyBoy(ROM, window="null")
pb.set_emulation_speed(0)
fails = []


def check(cond, msg):
    print(("PASS " if cond else "FAIL ") + msg)
    if not cond:
        fails.append(msg)


def ticks(n):
    for _ in range(n):
        if not pb.tick():
            raise RuntimeError("emulator stopped")


def press(btn):
    pb.button_press(btn)
    ticks(8)
    pb.button_release(btn)
    ticks(30)


def wait_state(s, timeout=600):
    for _ in range(timeout):
        if pb.memory[A["state"]] == s:
            return
        ticks(1)
    raise RuntimeError(f"never reached state {s}")


def close_text():
    for _ in range(6):
        if pb.memory[A["state"]] == ST_ROOM:
            return
        press("a")
    raise RuntimeError("text did not close")


def row_tiles(y):
    return [pb.memory[0x9800 + y * 32 + x] for x in range(20)]


def goto_hot(target):
    for _ in range(30):
        if pb.memory[A["hot"]] == target:
            return
        press("right")
    raise RuntimeError(f"cursor never reached hot {target}")


ticks(300)
check(pb.memory[A["state"]] == ST_TITLE, "boots to title")
press("start")
wait_state(ST_TEXT)
close_text()
check(pb.memory[A["state"]] == ST_ROOM, "in room")

# Fell the courtyard tree: poke room/hot/axe, USE, wood gained.
pb.memory[A["room"]] = 1
pb.memory[A["hot"]] = 1
pb.memory[A["items_lo"]] = 0x40  # AXE bit 6
press("a")
wait_state(ST_TEXT)
close_text()
check(pb.memory[A["items_hi"]] & 0x10, "WOOD gained from tree")
check(pb.memory[A["bridge"]] == 0, "no bridge yet")

# Jump to the chasm and force a redraw via the inventory screen.
pb.memory[A["room"]] = 11
pb.memory[A["hot"]] = 0
press("start")
wait_state(ST_INV)
press("b")
wait_state(ST_ROOM)
ticks(10)
before = row_tiles(5)
check(all(t == T_DARK for t in before[1:19]),
      f"gap row is dark before bridging (got {before[1:19]})")

# USE the wood, then read the row again: the log must be drawn.
press("a")
wait_state(ST_TEXT)
close_text()
check(pb.memory[A["bridge"]] == 1, "bridge flag set")
after = row_tiles(5)
check(all(t == T_BRIDGE for t in after[1:19]),
      f"bridge log spans the gap row (got {after[1:19]})")
check(after[0] == 1 and after[19] == 1,
      f"gap row walls untouched (got {after[0]}, {after[19]})")
img = pb.screen.image
img.resize((480, 432), resample=0).save(os.path.join(OUT, "15-bridge.png"))
print("shot: 15-bridge")

# Leave and re-enter through the wipe: the log must survive redraw.
goto_hot(3)  # observatory door
press("a")
ticks(30)
check(pb.memory[A["room"]] == 10, "observatory")
pb.memory[A["lens"]] = 1  # lens gate flag, so the door opens
goto_hot(2)  # lens gate
press("a")
ticks(30)
check(pb.memory[A["room"]] == 11, "chasm again")
again = row_tiles(5)
check(all(t == T_BRIDGE for t in again[1:19]),
      f"bridge persists after re-entry wipe (got {again[1:19]})")

print("BRIDGE QA", "FAILED" if fails else "PASSED",
      f"({len(fails)} failures)" if fails else "")
pb.stop()
sys.exit(1 if fails else 0)
