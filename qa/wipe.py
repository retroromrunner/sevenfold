#!/usr/bin/env python3
"""Headless PyBoy QA for the SEVENFOLD room-change screen wipe.

Proves the transition is a column-by-column reveal, not an instant cut:
after pressing A on the gatehouse door, the background tilemap must pass
through a frame where left columns already show the new room (courtyard)
while right columns still show the old room (gatehouse). Also captures a
mid-wipe screenshot.
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
A = dict(state=0xC0B1, room=0xC0B2, hot=0xC0B3, items=0xC0B7)

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


def tilemap():
    # Background map at 0x9800, 32 cols; screen shows cols 0-19, rows 0-17.
    return [[pb.memory[0x9800 + y * 32 + x] for x in range(20)]
            for y in range(13)]


def check(cond, msg):
    print(("PASS " if cond else "FAIL ") + msg)
    if not cond:
        raise AssertionError(msg)


def shot(name):
    img = pb.screen.image
    img.resize((480, 432), resample=0).save(os.path.join(OUT, name + ".png"))
    print(f"shot: {name} state={rd('state')} room={rd('room')}")


ticks(300)
check(rd("state") == ST_TITLE, "boots to title")
press("start")
wait_state(ST_TEXT)
close_text()
check(rd("state") == ST_ROOM and rd("room") == 0, "intro closes to gatehouse")
press("a")  # take key
close_text()
check(rd("items") & 0x01, "key in inventory")
check(rd("hot") == 1, "cursor on castle gate after take")

old = tilemap()
# Press A on the gate door and scan frame by frame for a mixed tilemap.
pb.button_press("a")


def changed_cols(m):
    return [x for x in range(20)
            if any(m[y][x] != old[y][x] for y in range(13))]


mixed = None
first_run = None
for _ in range(30):
    ticks(1)
    mid = tilemap()
    run = changed_cols(mid)
    if 3 <= len(run) <= 17:
        mixed = mid
        first_run = run
        shot("12-wipe-mid")
        break
second_run = None
if mixed is not None:
    ticks(5)
    second_run = changed_cols(tilemap())
pb.button_release("a")
ticks(40)
check(mixed is not None, "caught a mid-wipe frame (partial column change)")
if mixed is not None:
    # A column reveal shows as one contiguous run of changed columns
    # (columns identical between the two rooms, e.g. shared left-edge
    # wall tiles, legitimately show as unchanged), and the run must
    # extend further right a few frames later: left-to-right progress.
    check(first_run == list(range(min(first_run), max(first_run) + 1)),
          f"mid-wipe changed columns form one run {first_run}")
    check(second_run is not None and
          max(second_run) > max(first_run),
          f"wipe progresses rightward {first_run} -> {second_run}")
check(rd("room") == 1, "wipe completes into courtyard")
new = tilemap()
check(new != old, "final tilemap differs from old room")
shot("13-courtyard-after-wipe")

print("ALL WIPE QA CHECKS PASSED")
pb.stop()
