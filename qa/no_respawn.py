"""SEVENFOLD QA (round 5): taken items never respawn, and unlocked
passages stay open.

Regression test for Cori's round-4 bug report (used ladder/mirror
reappearing), extended to every round-5 item and lock: after taking
and consuming each item, revisiting its room must show a floor tile
and a never-cursor-reachable hotspot, and g_taken/g_taken2 bits must
survive consumption. Also asserts the persistent-unlock rule for the
castle gate, the ladder descent, the oil passage, and the choir loft
door, plus the stairs-stay-open rule (second descent with no ladder).

Route: the pure run with a revisit detour after every consumption.
Sins stay at 2, so floor tiles read as T_FLOOR (3), never the twisted
variant.
"""
import os, sys
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
sys.path.insert(0, os.path.expanduser(
    "~/workspace/gbc/tools/venv/lib/python3.12/site-packages"))
from pyboy import PyBoy

ROM = os.path.expanduser("~/workspace/sevenfold/releases/sevenfold.gb")

ST_TITLE, ST_ROOM, ST_TEXT, ST_SAC, ST_INV, ST_END = 0, 1, 2, 3, 4, 5
A = dict(state=0xC0B1, room=0xC0B2, hot=0xC0B3, sins=0xC0B4,
         gates=0xC0B5, pacts=0xC0B6, items_lo=0xC0B7, items_hi=0xC0B8,
         mirror=0xC0B9, taken_lo=0xC0C4, taken_hi=0xC0C5,
         locks_lo=0xC0C6, locks_hi=0xC0C7,
         items2_lo=0xC0CA, items2_hi=0xC0CB,
         taken2_lo=0xC0CC, taken2_hi=0xC0CD)

IT = dict(KEY=0, ROPE=1, CANDLE=2, MIRROR=3, BELL=4, LADDER=5, AXE=6,
          PRISM=7, OIL=8, HORN=9, OBSIDIAN=10, SUNSTONE=11, WOOD=12,
          BOW=13, STEM=14, FINALKEY=15, CHART=16, LOFTKEY=17)
LK_KEY, LK_LADDER, LK_OIL, LK_HORNKEY = 1, 9, 10, 13
T_FLOOR = 3

fails = []
def check(cond, msg):
    print(("PASS " if cond else "FAIL ") + msg)
    if not cond:
        fails.append(msg)

def tick(pb, n):
    for _ in range(n):
        pb.tick()

def press(pb, btn, hold=2):
    pb.button(btn, True); tick(pb, hold)
    pb.button(btn, False); tick(pb, 10)

def rd(pb, k):
    return pb.memory[A[k]]

def items(pb):
    return (rd(pb, "items_lo") | (rd(pb, "items_hi") << 8) |
            (rd(pb, "items2_lo") << 16) | (rd(pb, "items2_hi") << 24))

def taken(pb):
    return (rd(pb, "taken_lo") | (rd(pb, "taken_hi") << 8) |
            (rd(pb, "taken2_lo") << 16) | (rd(pb, "taken2_hi") << 24))

def locks(pb):
    return rd(pb, "locks_lo") | (rd(pb, "locks_hi") << 8)

def tile_at(pb, x, y):
    return pb.memory[0x9800 + y * 32 + x]

def wait_state(pb, st, n=200):
    for _ in range(n):
        if rd(pb, "state") == st:
            return True
        tick(pb, 1)
    return False

def close_text(pb):
    assert wait_state(pb, ST_TEXT), "expected ST_TEXT"
    for _ in range(6):
        if rd(pb, "state") != ST_TEXT:
            break
        press(pb, "a")
        tick(pb, 5)
    assert wait_state(pb, ST_ROOM), "expected return to ST_ROOM"

def interact(pb):
    press(pb, "a")

def goto_hot(pb, target):
    for _ in range(30):
        if rd(pb, "hot") == target:
            return
        press(pb, "right")
    assert rd(pb, "hot") == target, \
        f"cursor never reached hot {target} (stuck on {rd(pb, 'hot')})"

def take_at(pb, room, hot, bit, name):
    assert rd(pb, "room") == room, f"room {rd(pb,'room')} != {room}"
    goto_hot(pb, hot)
    interact(pb)
    close_text(pb)
    check(items(pb) & (1 << bit), f"took {name}")
    check(taken(pb) & (1 << bit), f"g_taken records {name}")

def go_door(pb, room, hot, target):
    assert rd(pb, "room") == room, f"room {rd(pb,'room')} != {room}"
    goto_hot(pb, hot)
    interact(pb)
    tick(pb, 30)
    check(rd(pb, "room") == target,
          f"door hot {hot} room {room} -> room {target} (got {rd(pb,'room')})")

def use_at(pb, room, hot):
    assert rd(pb, "room") == room, f"room {rd(pb,'room')} != {room}"
    goto_hot(pb, hot)
    interact(pb)
    close_text(pb)

def sacrifice(pb, room, hot, expect_sins):
    assert rd(pb, "room") == room, f"room {rd(pb,'room')} != {room}"
    goto_hot(pb, hot)
    interact(pb)
    assert wait_state(pb, ST_SAC), "expected ST_SAC"
    tick(pb, 10)
    press(pb, "a")
    close_text(pb)
    check(rd(pb, "sins") == expect_sins, f"sins == {expect_sins}")

def assert_gone(pb, room, hot, x, y, bit, name):
    """Tile is floor, hotspot never cursor-reachable, taken bit set,
    item absent from inventory (it was consumed)."""
    assert rd(pb, "room") == room, f"room {rd(pb,'room')} != {room}"
    check(tile_at(pb, x, y) == T_FLOOR,
          f"{name} tile is floor on revisit (got {tile_at(pb, x, y)})")
    seen = {rd(pb, "hot")}
    for _ in range(10):
        press(pb, "right")
        seen.add(rd(pb, "hot"))
    check(hot not in seen, f"{name} hotspot {hot} never cursor-reachable")
    check(taken(pb) & (1 << bit), f"g_taken {name} survives consumption")
    check(not (items(pb) & (1 << bit)), f"{name} still absent from inventory")

def main():
    pb = PyBoy(ROM, window="null")
    pb.set_emulation_speed(0)
    try:
        tick(pb, 90)
        check(rd(pb, "state") == ST_TITLE, "boots to title")
        press(pb, "start")
        assert wait_state(pb, ST_TEXT), "expected intro text"
        close_text(pb)

        # KEY: take, gate consumes it, g_taken bit must survive.
        take_at(pb, 0, 0, IT["KEY"], "KEY")
        go_door(pb, 0, 1, 1)
        check(not (items(pb) & (1 << IT["KEY"])), "KEY consumed at castle gate")
        check(locks(pb) & (1 << LK_KEY), "castle gate recorded in g_locks_open")

        # ROPE: shed at the end of the patio chain; well consumes it.
        go_door(pb, 1, 4, 19)
        go_door(pb, 19, 3, 20)
        go_door(pb, 20, 3, 21)
        take_at(pb, 21, 0, IT["ROPE"], "ROPE")
        go_door(pb, 21, 2, 20)
        go_door(pb, 20, 2, 19)
        go_door(pb, 19, 2, 1)
        use_at(pb, 1, 0)
        check(not (items(pb) & (1 << IT["ROPE"])), "ROPE consumed at well")
        check(items(pb) & (1 << IT["CANDLE"]), "CANDLE from well")
        # Revisit the shed: rope must be gone.
        go_door(pb, 1, 4, 19)
        go_door(pb, 19, 3, 20)
        go_door(pb, 20, 3, 21)
        assert_gone(pb, 21, 0, 3, 2, IT["ROPE"], "rope")
        go_door(pb, 21, 2, 20)
        go_door(pb, 20, 2, 19)
        go_door(pb, 19, 2, 1)
        go_door(pb, 1, 2, 2)

        # MIRROR + OIL (cellar at the end of the pantry chain).
        go_door(pb, 2, 0, 3)
        take_at(pb, 3, 0, IT["MIRROR"], "MIRROR")
        go_door(pb, 3, 3, 17)
        go_door(pb, 17, 2, 29)
        go_door(pb, 29, 2, 30)
        take_at(pb, 30, 0, IT["OIL"], "OIL")
        go_door(pb, 30, 2, 29)
        go_door(pb, 29, 1, 17)
        go_door(pb, 17, 1, 3)
        go_door(pb, 3, 2, 2)

        # Gates -> chapel -> gallery (2 sacrifices, no pacts).
        sacrifice(pb, 2, 2, 1)
        go_door(pb, 2, 2, 5)
        sacrifice(pb, 5, 3, 2)
        go_door(pb, 5, 3, 7)

        # LADDER + AXE, then glass consumes MIRROR.
        take_at(pb, 7, 3, IT["LADDER"], "LADDER")
        go_door(pb, 7, 5, 18)
        take_at(pb, 18, 0, IT["AXE"], "AXE")
        go_door(pb, 18, 1, 7)
        use_at(pb, 7, 0)
        check(rd(pb, "mirror") == 1, "mirror used at glass")
        check(not (items(pb) & (1 << IT["MIRROR"])), "MIRROR consumed at glass")
        # Revisit the kitchen: mirror must be gone.
        go_door(pb, 7, 4, 5)
        go_door(pb, 5, 4, 2)
        go_door(pb, 2, 0, 3)
        assert_gone(pb, 3, 0, 8, 2, IT["MIRROR"], "mirror")
        go_door(pb, 3, 2, 2)
        go_door(pb, 2, 2, 5)
        go_door(pb, 5, 3, 7)

        # Descent consumes LADDER; up and down again with no ladder.
        go_door(pb, 7, 4, 5)
        go_door(pb, 5, 2, 6)
        check(not (items(pb) & (1 << IT["LADDER"])), "LADDER consumed at descent")
        check(locks(pb) & (1 << LK_LADDER), "descent recorded in g_locks_open")
        take_at(pb, 6, 0, IT["BELL"], "BELL")
        use_at(pb, 6, 4)
        check(not (items(pb) & (1 << IT["CANDLE"])), "CANDLE consumed at coffer")
        go_door(pb, 6, 2, 5)
        go_door(pb, 5, 2, 6)
        check(rd(pb, "room") == 6,
              "stairs stay open: second descent without ladder reaches crypt")
        # Back to the gatehouse: key must be gone, gate re-opens.
        go_door(pb, 6, 2, 5)
        go_door(pb, 5, 4, 2)
        go_door(pb, 2, 3, 1)
        go_door(pb, 1, 3, 0)
        assert_gone(pb, 0, 0, 5, 2, IT["KEY"], "key")
        go_door(pb, 0, 1, 1)
        check(rd(pb, "room") == 1, "castle gate re-opens without key")
        go_door(pb, 1, 2, 2)
        go_door(pb, 2, 2, 5)
        go_door(pb, 5, 3, 7)
        # Ladder must still be gone after the descents.
        assert_gone(pb, 7, 3, 16, 2, IT["LADDER"], "ladder")

        # Stair guardian consumes BELL; observatory chart room.
        go_door(pb, 7, 1, 8)
        use_at(pb, 8, 0)
        check(not (items(pb) & (1 << IT["BELL"])), "BELL consumed at guardian")
        go_door(pb, 8, 1, 10)
        go_door(pb, 10, 4, 22)
        take_at(pb, 22, 0, IT["CHART"], "STAR CHART")
        go_door(pb, 22, 2, 10)
        go_door(pb, 10, 3, 8)
        go_door(pb, 8, 2, 7)
        go_door(pb, 7, 4, 5)
        go_door(pb, 5, 4, 2)
        go_door(pb, 2, 1, 4)
        take_at(pb, 4, 1, IT["PRISM"], "PRISM")
        check(not (items(pb) & (1 << IT["CHART"])), "CHART consumed at prism")
        # Revisit the chart room: chart must be gone.
        go_door(pb, 4, 2, 2)
        go_door(pb, 2, 2, 5)
        go_door(pb, 5, 3, 7)
        go_door(pb, 7, 1, 8)
        go_door(pb, 8, 1, 10)
        go_door(pb, 10, 4, 22)
        assert_gone(pb, 22, 0, 3, 2, IT["CHART"], "star chart")
        go_door(pb, 22, 2, 10)

        # Lens consumes PRISM; chasm backtrack for wood.
        use_at(pb, 10, 0)
        check(not (items(pb) & (1 << IT["PRISM"])), "PRISM consumed at lens")
        go_door(pb, 10, 2, 11)
        go_door(pb, 11, 3, 10)
        go_door(pb, 10, 3, 8)
        go_door(pb, 8, 2, 7)
        go_door(pb, 7, 4, 5)
        go_door(pb, 5, 4, 2)
        go_door(pb, 2, 3, 1)
        use_at(pb, 1, 1)
        check(not (items(pb) & (1 << IT["AXE"])), "AXE consumed at tree")
        go_door(pb, 1, 2, 2)
        go_door(pb, 2, 2, 5)
        go_door(pb, 5, 3, 7)
        go_door(pb, 7, 1, 8)
        go_door(pb, 8, 1, 10)
        go_door(pb, 10, 2, 11)
        use_at(pb, 11, 0)
        check(not (items(pb) & (1 << IT["WOOD"])), "WOOD consumed at chasm")
        go_door(pb, 11, 1, 12)

        # Smithy BOW, then passage consumes OIL.
        go_door(pb, 12, 3, 15)
        take_at(pb, 15, 0, IT["BOW"], "KEY BOW")
        go_door(pb, 15, 2, 12)
        go_door(pb, 12, 0, 13)
        check(not (items(pb) & (1 << IT["OIL"])), "OIL consumed at passage")
        check(locks(pb) & (1 << LK_OIL), "passage recorded in g_locks_open")
        # Revisit the cellar: oil must be gone, then the passage must
        # still let us back through without oil.
        go_door(pb, 13, 3, 12)
        go_door(pb, 12, 2, 11)
        go_door(pb, 11, 3, 10)
        go_door(pb, 10, 3, 8)
        go_door(pb, 8, 2, 7)
        go_door(pb, 7, 4, 5)
        go_door(pb, 5, 4, 2)
        go_door(pb, 2, 0, 3)
        go_door(pb, 3, 3, 17)
        go_door(pb, 17, 2, 29)
        go_door(pb, 29, 2, 30)
        assert_gone(pb, 30, 0, 3, 2, IT["OIL"], "oil")
        go_door(pb, 30, 2, 29)
        go_door(pb, 29, 1, 17)
        go_door(pb, 17, 1, 3)
        go_door(pb, 3, 2, 2)
        go_door(pb, 2, 2, 5)
        go_door(pb, 5, 3, 7)
        go_door(pb, 7, 1, 8)
        go_door(pb, 8, 1, 10)
        go_door(pb, 10, 2, 11)
        go_door(pb, 11, 1, 12)
        go_door(pb, 12, 0, 13)
        check(rd(pb, "room") == 13, "oil passage stays open without oil")

        # Passage chain: LOFT KEY, choir door consumes it, HORN.
        go_door(pb, 13, 6, 24)
        go_door(pb, 24, 2, 25)
        go_door(pb, 25, 2, 26)
        go_door(pb, 26, 2, 27)
        go_door(pb, 27, 2, 28)
        take_at(pb, 28, 0, IT["LOFTKEY"], "LOFT KEY")
        go_door(pb, 28, 2, 27)
        go_door(pb, 27, 1, 26)
        go_door(pb, 26, 1, 25)
        go_door(pb, 25, 1, 24)
        go_door(pb, 24, 1, 13)
        go_door(pb, 13, 5, 23)
        check(not (items(pb) & (1 << IT["LOFTKEY"])),
              "LOFT KEY consumed at choir loft door")
        check(locks(pb) & (1 << LK_HORNKEY),
              "choir loft door recorded in g_locks_open")
        take_at(pb, 23, 0, IT["HORN"], "HORN")
        # Revisit the deep cell: loft key must be gone.
        go_door(pb, 23, 2, 13)
        go_door(pb, 13, 6, 24)
        go_door(pb, 24, 2, 25)
        go_door(pb, 25, 2, 26)
        go_door(pb, 26, 2, 27)
        go_door(pb, 27, 2, 28)
        assert_gone(pb, 28, 0, 3, 2, IT["LOFTKEY"], "loft key")
        go_door(pb, 28, 2, 27)
        go_door(pb, 27, 1, 26)
        go_door(pb, 26, 1, 25)
        go_door(pb, 25, 1, 24)
        go_door(pb, 24, 1, 13)
        # Gatekeeper consumes HORN; revisit the choir loft: horn gone,
        # and the loft door stays open without the key.
        use_at(pb, 13, 0)
        check(not (items(pb) & (1 << IT["HORN"])), "HORN consumed at gatekeeper")
        go_door(pb, 13, 5, 23)
        check(rd(pb, "room") == 23, "choir loft door stays open without key")
        assert_gone(pb, 23, 0, 3, 2, IT["HORN"], "horn")
        go_door(pb, 23, 2, 13)

        # Finish: belfry STEM, forge FINAL KEY, sanctum, exit.
        go_door(pb, 13, 4, 16)
        take_at(pb, 16, 0, IT["STEM"], "KEY STEM")
        go_door(pb, 16, 1, 13)
        go_door(pb, 13, 3, 12)
        go_door(pb, 12, 3, 15)
        use_at(pb, 15, 1)
        check(items(pb) & (1 << IT["FINALKEY"]), "FINAL KEY from forge")
        go_door(pb, 15, 2, 12)
        go_door(pb, 12, 0, 13)
        go_door(pb, 13, 2, 14)
        go_door(pb, 14, 0, 9)
        check(items(pb) == (1 << IT["FINALKEY"]),
              f"final inventory is FINAL KEY alone (got {items(pb):#x})")
        goto_hot(pb, 0)
        interact(pb)
        assert wait_state(pb, ST_END), "expected ST_END"
        check(rd(pb, "sins") == 2, "pure ending at 2 sins")

        print("NO-RESPAWN QA", "FAILED" if fails else "PASSED",
              f"({len(fails)} failures)" if fails else "")
        return 1 if fails else 0
    finally:
        pb.stop()

if __name__ == "__main__":
    sys.exit(main())
