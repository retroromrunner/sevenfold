#!/usr/bin/env python3
"""SEVENFOLD audio register QA (headless QA cannot hear sound, so this
asserts the engine is actually wired and running).

Music is TITLE-SCREEN ONLY (round-2 fix: in-game music annoyed Cori).
So this script asserts both halves:
- On the title screen: NR52 sound power on with channel 2 (music)
  active, NR51 routes music (ch2) and SFX (ch1) to both speakers
  (0x33), NR22 reads back 0x70, the exact envelope music_tick writes
  (an unwritten channel reads back 0xBF).
- In a room after game start: NR22 reads back 0x00, proving the music
  engine stopped dead and stays off in-game.
- A pickup SFX still drives NR12 to 0xF0 while sounding and back to
  0x00 when its sequence ends, proving the SFX engine runs and
  finishes in-game, and NR51 is untouched by SFX (channel separation).

Findings from probing: on real hardware AND in PyBoy, NR23/NR24 (freq
registers) are write-only and read back as masks, so note-by-note
melody advance cannot be observed headless -- that part is honestly
left to Cori's ears. The wipe arpeggio likewise cannot be heard
headless; its sequence is asserted only by the fact that room moves
still complete (wipe.py + playthroughs) and SFX registers behave.

Registers: NR12=0xFF12, NR22=0xFF17, NR51=0xFF25, NR52=0xFF26.
"""
import os
import sys

sys.path.insert(0, os.path.expanduser(
    "~/workspace/gbc/tools/venv/lib/python3.12/site-packages"))
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
from pyboy import PyBoy

ROM = os.path.expanduser("~/workspace/sevenfold/releases/sevenfold.gb")
ST_TITLE, ST_ROOM, ST_TEXT = 0, 1, 2

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


def check(cond, msg):
    print(("PASS " if cond else "FAIL ") + msg)
    if not cond:
        raise AssertionError(msg)


ticks(300)
check(pb.memory[0xFF26] & 0x80, "sound power on (NR52 bit 7)")
check(pb.memory[0xFF26] & 0x02, "music channel 2 active (NR52 bit 1)")
check(pb.memory[0xFF25] == 0x33, f"music+SFX routed (NR51={pb.memory[0xFF25]:#04x})")
check(pb.memory[0xFF17] == 0x70, f"music envelope written (NR22={pb.memory[0xFF17]:#04x})")

# Intro, then prove the music stopped once the game started: after the
# intro text closes into the gatehouse, the title music envelope must
# read back silent, and stay silent in-game.
press("start")
for _ in range(600):
    if pb.memory[0xC0B1] == ST_TEXT:
        break
    ticks(1)
for _ in range(6):
    if pb.memory[0xC0B1] == ST_ROOM:
        break
    press("a")
ticks(60)
check(pb.memory[0xFF17] == 0x00,
      f"music silenced in room (NR22={pb.memory[0xFF17]:#04x})")
pb.button_press("a")
ticks(6)
check(pb.memory[0xFF12] == 0xF0, f"pickup SFX sounding (NR12={pb.memory[0xFF12]:#04x})")
pb.button_release("a")
ticks(60)
check(pb.memory[0xFF12] == 0x00, f"pickup SFX finished silent (NR12={pb.memory[0xFF12]:#04x})")
check(pb.memory[0xFF25] == 0x33, "NR51 still routes both channels after SFX")
check(pb.memory[0xFF26] & 0x80, "sound still powered after SFX")

# Wipe sound: close the take-key text, then walk through the castle
# gate. While the column wipe runs, the wipe arpeggio must be sounding
# on channel 1 (NR12=0xF0); afterwards the move completes.
for _ in range(6):
    if pb.memory[0xC0B1] == ST_ROOM:
        break
    press("a")
check(pb.memory[0xC0B1] == ST_ROOM, "back in room after take text")
pb.button_press("a")
ticks(6)
check(pb.memory[0xFF12] == 0xF0,
      f"wipe SFX sounding mid-wipe (NR12={pb.memory[0xFF12]:#04x})")
pb.button_release("a")
ticks(60)
check(pb.memory[0xC0B2] == 1, "wipe completes into courtyard")

# Round-5 ending tracks: poke the game straight to the final gate for
# each sin band and prove the ending screen drives channel 2 with the
# right track selected (g_end_track 1 pure / 2 tainted / 3 keeper).
# Melody content itself cannot be heard headless; the track selection
# plus a live channel-2 envelope is the honest register-level proof.
for sins, track, name in ((2, 1, "pure"), (4, 2, "tainted"), (7, 3, "keeper")):
    pb.memory[0xC0B2] = 9    # vestibule
    pb.memory[0xC0B3] = 0    # the final gate
    pb.memory[0xC0B7] = 0x00
    pb.memory[0xC0B8] = 0x80  # FINAL KEY bit 15
    pb.memory[0xC0CA] = 0x00
    pb.memory[0xC0CB] = 0x00
    pb.memory[0xC0B4] = sins
    press("a")
    for _ in range(600):
        if pb.memory[0xC0B1] == 5:  # ST_END
            break
        ticks(1)
    check(pb.memory[0xC0B1] == 5, f"{name} ending screen reached")
    ticks(60)
    check(pb.memory[0xFF17] == 0x70,
          f"{name} ending drives channel 2 (NR22={pb.memory[0xFF17]:#04x})")
    check(pb.memory[0xC0CE] == track,
          f"{name} ending track selected (g_end_track={pb.memory[0xC0CE]})")
    # Back to title, then start a fresh game for the next band.
    press("a")
    for _ in range(600):
        if pb.memory[0xC0B1] == 0:  # ST_TITLE
            break
        ticks(1)
    check(pb.memory[0xC0B1] == 0, f"back to title after {name} ending")
    press("start")
    for _ in range(600):
        if pb.memory[0xC0B1] == 2:  # ST_TEXT (intro)
            break
        ticks(1)
    for _ in range(6):
        if pb.memory[0xC0B1] == 1:  # ST_ROOM
            break
        press("a")
    check(pb.memory[0xC0B1] == 1, "fresh game started for next band")

print("ALL AUDIO REGISTER QA CHECKS PASSED")
pb.stop()
