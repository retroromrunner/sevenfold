#!/usr/bin/env python3
"""SEVENFOLD pointer QA (round-2 fix): prove the hotspot cursor sprite
sits immediately adjacent to the hotspot tile it points to.

Root cause of the old complaint: GBDK's move_sprite takes screen coords
MINUS 8 (x) and MINUS 16 (y), so passing raw tile pixels put the
diamond a full tile left and two tiles up from the item. The fix places
it one tile left of the hotspot, same row, edge touching edge.

Probe: boot to the gatehouse (hotspot 0 = RUSTY KEY at tile (5, 2)),
wait for a blink-visible frame (g_frame bit 4 clear), then read OAM
sprite 0 (base 0xFE00: Y, X, tile) and convert to screen coords
(screen_x = OAM_X - 8, screen_y = OAM_Y - 16). The key tile spans
pixels x 40..47, y 16..23, so the adjacent-left sprite top-left must
be (32, 16). Also captures 14-pointer.png.
"""
import os
import sys

sys.path.insert(0, os.path.expanduser(
    "~/workspace/gbc/tools/venv/lib/python3.12/site-packages"))
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
from pyboy import PyBoy

ROM = os.path.expanduser("~/workspace/sevenfold/releases/sevenfold.gb")
OUT = os.path.expanduser("~/workspace/sevenfold/qa")
os.makedirs(OUT, exist_ok=True)

ST_TITLE, ST_ROOM, ST_TEXT = 0, 1, 2
A = dict(state=0xC0B1, room=0xC0B2, hot=0xC0B3, frame=0xC0BF)

pb = PyBoy(ROM, window="null")
pb.set_emulation_speed(0)


def rd(n):
    return pb.memory[A[n]]


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
        if rd("state") == s:
            return
        ticks(1)
    raise RuntimeError(f"never reached state {s} (now {rd('state')})")


def close_text():
    for _ in range(6):
        if rd("state") == ST_ROOM:
            return
        press("a")
    raise RuntimeError("text did not close")


def check(cond, msg):
    print(("PASS " if cond else "FAIL ") + msg)
    if not cond:
        raise AssertionError(msg)


ticks(300)
check(rd("state") == ST_TITLE, "boots to title")
press("start")
wait_state(ST_TEXT)
close_text()
check(rd("state") == ST_ROOM and rd("room") == 0, "gatehouse")
check(rd("hot") == 0, "cursor on RUSTY KEY hotspot (tile 5, 2)")

# Wait for a blink-visible frame EARLY in its 16-frame window, then tick
# a few more frames: GBDK writes shadow OAM during the frame and real
# OAM only updates via DMA at the next vblank, so reading on the first
# visible frame still shows the previous (hidden) DMA copy.
for _ in range(64):
    if (rd("frame") & 31) < 12:
        break
    ticks(1)
check((rd("frame") & 31) < 12, "caught an early blink-visible frame")
ticks(3)
check(not (rd("frame") & 16), "still in blink-visible window")

oam_y, oam_x, oam_tile = pb.memory[0xFE00], pb.memory[0xFE01], pb.memory[0xFE02]
sx, sy = oam_x - 8, oam_y - 16
print(f"sprite OAM=({oam_x},{oam_y}) tile={oam_tile} -> screen=({sx},{sy})")
check(oam_tile == 200, f"cursor tile is the diamond (got {oam_tile})")
check((sx, sy) == (32, 16),
      f"cursor adjacent-left of key tile (screen {sx},{sy}, want (32,16))")

img = pb.screen.image
img.resize((480, 432), resample=0).save(os.path.join(OUT, "14-pointer.png"))
print("shot: 14-pointer")

print("ALL POINTER QA CHECKS PASSED")
pb.stop()
