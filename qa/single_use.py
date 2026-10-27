"""SEVENFOLD single-use QA: a denied attempt must consume nothing, and
an item used at its obstacle must be gone afterwards.

1. Castle gate without the key -> denied, inventory still empty, key
   still takeable; after taking it, the gate consumes it.
2. Well without rope -> denied, inventory unchanged; after fetching
   the rope from the shed (round-5: patio -> garden -> shed) and
   returning, the well consumes it and yields the candle.
3. Prism without the star chart (round-5 take lock) -> denied,
   inventory unchanged, prism still hidden.
"""
import os
import sys

sys.path.insert(0, os.path.expanduser(
    "~/workspace/gbc/tools/venv/lib/python3.12/site-packages"))
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
from pyboy import PyBoy

ROM = os.path.expanduser("~/workspace/sevenfold/releases/sevenfold.gb")
ST_TITLE, ST_ROOM, ST_TEXT = 0, 1, 2
A = dict(state=0xC0B1, room=0xC0B2, hot=0xC0B3, items_lo=0xC0B7,
         items_hi=0xC0B8)

pb = PyBoy(ROM, window="null")
pb.set_emulation_speed(0)
fails = []


def rd(n):
    return pb.memory[A[n]]


def items():
    return rd("items_lo") | (rd("items_hi") << 8)


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


def right(n=1):
    for _ in range(n):
        press("right")


def check(cond, msg):
    print(("PASS " if cond else "FAIL ") + msg)
    if not cond:
        fails.append(msg)


ticks(300)
check(rd("state") == ST_TITLE, "boots to title")
press("start")
wait_state(ST_TEXT)
close_text()
check(rd("room") == 0, "gatehouse")

# 1. Gate without key: denied, nothing consumed, key still there.
press("right")
check(rd("hot") == 1, "cursor on castle gate")
press("a")
wait_state(ST_TEXT)
check(rd("room") == 0, "denied gate does not move rooms")
close_text()
check(items() == 0, "denied gate consumed nothing")
press("left")
press("a")
close_text()
check(items() & 0x01, "key still takeable after denial")
press("a")
ticks(30)
check(rd("room") == 1, "gate opens with key")
check(not (items() & 0x01), "key consumed at gate (single use)")

# 2. Well without rope: denied, inventory unchanged. The cursor lands
# on the well on entering the courtyard.
check(rd("hot") == 0, "cursor on well")
press("a")
wait_state(ST_TEXT)
close_text()
check(items() == 0, "denied well consumed nothing")

# Fetch the rope: patio -> garden -> shed, then all the way back.
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
check(items() & 0x02, "rope taken")
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
check(not (items() & 0x02), "rope consumed at well (single use)")
check(items() & 0x04, "candle gained from well")

# 3. Prism without the star chart: denied, inventory unchanged.
right(1)  # cursor: tree -> courtyard gate
press("a")
check(rd("room") == 2, "great hall")
right(1)  # east door
press("a")
check(rd("room") == 4, "library")
right(1)  # prism
press("a")
wait_state(ST_TEXT)
close_text()
check(not (items() & 0x80), "denied prism take gains no prism")
check(items() & 0x04, "denied prism consumed nothing (candle kept)")

print("SINGLE-USE QA", "FAILED" if fails else "PASSED")
pb.stop()
sys.exit(1 if fails else 0)
