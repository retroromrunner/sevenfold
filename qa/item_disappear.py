"""SEVENFOLD pickup-order QA (round-5 fix): prove a picked-up item's
sprite is hidden BEFORE the pickup message displays. Reads the
background tilemap in VRAM (0x9800) at the item's tile position WHILE
the pickup message is still on screen (ST_TEXT, before any close).
Tile indices from src/tiles.h: T_FLOOR=3, T_KEY=11, T_MIRROR=13.
Round-5 note: the rope moved out of the courtyard to the shed at the
end of the patio/garden chain, so reaching the kitchen now goes
courtyard -> patio -> garden -> shed (rope) -> back -> well -> hall.
"""
import os
import sys

sys.path.insert(0, os.path.expanduser(
    "~/workspace/gbc/tools/venv/lib/python3.12/site-packages"))
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
from pyboy import PyBoy

ROM = os.path.expanduser("~/workspace/sevenfold/releases/sevenfold.gb")
OUT = os.path.expanduser("~/workspace/sevenfold/qa")
ST_TITLE, ST_ROOM, ST_TEXT = 0, 1, 2
A = dict(state=0xC0B1, room=0xC0B2, hot=0xC0B3, items_lo=0xC0B7)

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
    raise RuntimeError(f"never reached state {s}")


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


def tile_at(x, y):
    return pb.memory[0x9800 + y * 32 + x]


def shot(name):
    img = pb.screen.image
    img.resize((480, 432), resample=0).save(os.path.join(OUT, name + ".png"))
    print(f"shot: {name} room={rd('room')} hot={rd('hot')} state={rd('state')}")


def right(n=1):
    for _ in range(n):
        press("right")


ticks(300)
check(rd("state") == ST_TITLE, "boots to title")
press("start")
wait_state(ST_TEXT)
close_text()
check(rd("room") == 0, "gatehouse")

# Key: present before, GONE WHILE the pickup message is on screen.
check(tile_at(5, 2) == 11, f"key tile present before pickup (got {tile_at(5, 2)})")
press("a")  # take key
wait_state(ST_TEXT)
check(tile_at(5, 2) == 3,
      f"key tile ALREADY gone while pickup message shows (got {tile_at(5, 2)})")
shot("10-key-gone")
close_text()
check(rd("items_lo") & 0x01, "key in inventory")

# On to the kitchen: gate -> courtyard, well denied (no rope yet),
# patio -> garden -> shed for the rope, back, well -> candle, hall.
press("a")  # castle gate -> courtyard
check(rd("room") == 1, "courtyard")
press("a")  # well denied (cursor starts on the well)
close_text()
right(4)  # patio door
press("a")
check(rd("room") == 19, "patio")
right(3)  # garden door
press("a")
check(rd("room") == 20, "garden")
right(3)  # shed door
press("a")
check(rd("room") == 21, "shed")
press("a")  # take rope
close_text()
right(1)  # garden door (cursor sits on tools after the take)
press("a")
check(rd("room") == 20, "garden again")
right(2)  # patio door
press("a")
check(rd("room") == 19, "patio again")
right(2)  # courtyard door
press("a")
check(rd("room") == 1, "courtyard again")
press("a")  # well -> candle
close_text()
right(1)  # cursor: tree -> courtyard gate
press("a")  # courtyard gate -> hall
check(rd("room") == 2, "great hall")
press("a")  # west door -> kitchen
check(rd("room") == 3, "kitchen")
check(tile_at(8, 2) == 13, f"mirror tile present before pickup (got {tile_at(8, 2)})")
press("a")  # take mirror
wait_state(ST_TEXT)
check(tile_at(8, 2) == 3,
      f"mirror tile ALREADY gone while pickup message shows (got {tile_at(8, 2)})")
shot("11-mirror-gone")
close_text()
check(rd("items_lo") & 0x08, "mirror in inventory")

print("ALL PICKUP-ORDER QA CHECKS PASSED")
pb.stop()
