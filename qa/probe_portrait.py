#!/usr/bin/env python3
"""Probe: inventory/portrait at 7 sins (darkest corruption stage).
Boots, enters the game, then pokes WRAM sins/virtues before opening the
inventory, so the portrait stage can be screenshotted without a full run.
"""
import os
import sys

sys.path.insert(0, os.path.expanduser(
    "~/workspace/gbc/tools/venv/lib/python3.12/site-packages"))
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
from pyboy import PyBoy

ROM = os.path.expanduser("~/workspace/sevenfold/releases/sevenfold.gb")
OUT = os.path.expanduser("~/workspace/sevenfold/qa")

ST_ROOM, ST_TEXT, ST_INV = 1, 2, 4
A = dict(state=0xC0B1, sins=0xC0B4, items=0xC0B7, virtues=0xC0BB)

pb = PyBoy(ROM, window="null")
pb.set_emulation_speed(0)


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


ticks(300)
press("start")
wait_state(ST_TEXT)
for _ in range(6):
    if pb.memory[A["state"]] == ST_ROOM:
        break
    press("a")
assert pb.memory[A["state"]] == ST_ROOM, "did not reach room"

pb.memory[A["sins"]] = 7
pb.memory[A["virtues"]] = 0
pb.memory[A["items"]] = 0x1F
press("start")
wait_state(ST_INV)
img = pb.screen.image
img.resize((480, 432), resample=0).save(os.path.join(OUT, "08-portrait-dark.png"))
print("shot: 08-portrait-dark (sins poked to 7)")
pb.stop()
