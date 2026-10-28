#!/usr/bin/env python3
"""SEVENFOLD sacrifice-sting QA.

A required-gate sacrifice must play the new sinister harmonic-minor
sting (sfx_gate_sting, ~132 frames), NOT the old two-tone
sfx_sacrifice (~32 frames). Headless proof: poke the game to the
hall's sealed gate, confirm a sacrifice, and sample channel 1.
At ~72 frames after the confirm the old sound would be long over;
the sting must still be sounding (NR12=0xF0). By ~232 frames the
sting must have finished (NR12=0x00) and the gate flag must be set.
"""
import os
import sys

sys.path.insert(0, os.path.expanduser(
    "~/workspace/gbc/tools/venv/lib/python3.12/site-packages"))
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
from pyboy import PyBoy

ROM = os.path.expanduser("~/workspace/sevenfold/releases/sevenfold.gb")
ST_ROOM, ST_TEXT, ST_SAC = 1, 2, 3
A = dict(state=0xC0B1, room=0xC0B2, hot=0xC0B3, sins=0xC0B4,
         gates=0xC0B5, virtues=0xC0BB)

pb = PyBoy(ROM, window="null")
pb.set_emulation_speed(0)


def ticks(n):
    for _ in range(n):
        if not pb.tick():
            raise RuntimeError("emulator stopped")


def press(btn):
    pb.button_press(btn)
    ticks(2)
    pb.button_release(btn)
    ticks(10)


def check(cond, msg):
    print(("PASS " if cond else "FAIL ") + msg)
    if not cond:
        raise AssertionError(msg)


try:
    ticks(90)
    press("start")
    for _ in range(600):
        if pb.memory[A["state"]] == ST_TEXT:
            break
        ticks(1)
    for _ in range(6):
        if pb.memory[A["state"]] == ST_ROOM:
            break
        press("a")
    check(pb.memory[A["state"]] == ST_ROOM, "game started")

    # Poke straight to the hall's sealed gate (gate 0, no item req).
    pb.memory[A["room"]] = 2
    pb.memory[A["hot"]] = 2
    pb.memory[A["virtues"]] = 0x7F
    pb.memory[A["sins"]] = 0
    pb.memory[A["gates"]] = 0
    press("a")
    for _ in range(300):
        if pb.memory[A["state"]] == ST_SAC:
            break
        ticks(1)
    check(pb.memory[A["state"]] == ST_SAC, "sacrifice menu opened")
    ticks(10)
    press("a")  # confirm: gate opens, sting starts
    for _ in range(300):
        if pb.memory[A["state"]] == ST_TEXT:
            break
        ticks(1)
    check(pb.memory[A["state"]] == ST_TEXT, "offer message shown")
    check(pb.memory[A["gates"]] & 0x01, "gate 0 opened by sacrifice")

    # ~72 frames after the confirm: the old two-tone (~32 frames)
    # would be silent; the sting (~132 frames) must still sound.
    ticks(60)
    check(pb.memory[0xFF12] == 0xF0,
          f"gate sting still sounding at ~72 frames (NR12={pb.memory[0xFF12]:#04x})")
    # ~232 frames after the confirm: the sting must be done, and it
    # must have silenced channel 1 rather than hanging.
    ticks(160)
    check(pb.memory[0xFF12] == 0x00,
          f"gate sting finished by ~232 frames (NR12={pb.memory[0xFF12]:#04x})")
    print("ALL SACRIFICE-STING QA CHECKS PASSED")
finally:
    pb.stop()
