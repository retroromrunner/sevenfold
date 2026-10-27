"""SEVENFOLD QA: pure run (2 sins, no pacts) end to end, round-5 economy.

Verifies every single-use item: each is gained once and its bit is
CLEAR after its one obstacle, so no item gates two obstacles. Final
inventory must be FINAL KEY alone. Rooms/hotspot indices match the
round-5 layout in src/main.c (31 rooms; patio/garden/shed rope chain,
chart-room prism gate, passageway loft-key chain, larder/cellar oil
chain). The long backtracks are exercised: rope from the shed to the
courtyard well, chart from the observatory wing back to the library
prism, wood from the courtyard tree back to the chasm, loft key from
the deep cell back to the choir loft and the horn back to the
gatekeeper, and bow + stem + sunstone to the smithy forge.
"""
import os, sys
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
sys.path.insert(0, os.path.expanduser(
    "~/workspace/gbc/tools/venv/lib/python3.12/site-packages"))
from pyboy import PyBoy

ROM = os.path.expanduser("~/workspace/sevenfold/releases/sevenfold.gb")
OUT = os.path.expanduser("~/workspace/sevenfold/qa")
os.makedirs(OUT, exist_ok=True)

ST_TITLE, ST_ROOM, ST_TEXT, ST_SAC, ST_INV, ST_END = 0, 1, 2, 3, 4, 5
# rooms: 0 gatehouse 1 courtyard 2 hall 3 kitchen 4 library 5 chapel
# 6 crypt 7 gallery 8 stair 9 vestibule 10 observatory 11 chasm
# 12 archive 13 echo 14 reliquary 15 smithy 16 belfry 17 pantry
# 18 armory 19 patio 20 garden 21 shed 22 chartroom 23 choir
# 24 pass1 25 pass2 26 pass3 27 pass4 28 deepcell 29 larder 30 cellar
# Items 0-15 in g_items (0xC0B7-8), items 16-17 in g_items2 (0xC0CA-B);
# g_taken 0xC0C4-5, g_taken2 0xC0CC-D, g_end_track 0xC0CE.
A = dict(state=0xC0B1, room=0xC0B2, hot=0xC0B3, sins=0xC0B4,
         gates=0xC0B5, pacts=0xC0B6, items_lo=0xC0B7, items_hi=0xC0B8,
         mirror=0xC0B9, bell=0xC0BA, virtues=0xC0BB,
         sac_sel=0xC0BE, lens=0xC0C0, echo=0xC0C1, bridge=0xC0C2,
         taken_lo=0xC0C4, taken_hi=0xC0C5, locks_lo=0xC0C6,
         coffer=0xC0C8, tree=0xC0C9, items2_lo=0xC0CA, items2_hi=0xC0CB,
         end_track=0xC0CE)

IT = dict(KEY=0, ROPE=1, CANDLE=2, MIRROR=3, BELL=4, LADDER=5, AXE=6,
          PRISM=7, OIL=8, HORN=9, OBSIDIAN=10, SUNSTONE=11, WOOD=12,
          BOW=13, STEM=14, FINALKEY=15, CHART=16, LOFTKEY=17)

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
    """Cycle RIGHT until the cursor sits on hotspot index target."""
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

def denied_at(pb, room, hot, name):
    """Interact expecting a denied message: nothing is consumed."""
    assert rd(pb, "room") == room, f"room {rd(pb,'room')} != {room}"
    before = items(pb)
    goto_hot(pb, hot)
    interact(pb)
    close_text(pb)
    check(items(pb) == before, f"denied {name} consumes nothing")

def sacrifice(pb, room, hot, expect_sins):
    assert rd(pb, "room") == room, f"room {rd(pb,'room')} != {room}"
    goto_hot(pb, hot)
    interact(pb)
    assert wait_state(pb, ST_SAC), "expected ST_SAC"
    tick(pb, 10)
    press(pb, "a")
    close_text(pb)
    check(rd(pb, "sins") == expect_sins, f"sins == {expect_sins}")

def main():
    pb = PyBoy(ROM, window="null")
    pb.set_emulation_speed(0)
    try:
        tick(pb, 90)
        check(rd(pb, "state") == ST_TITLE, "boots to title")
        press(pb, "start")
        assert wait_state(pb, ST_TEXT), "expected intro text"
        close_text(pb)
        check(rd(pb, "room") == 0, "starts in gatehouse")

        # Gatehouse: KEY, then castle gate consumes it.
        take_at(pb, 0, 0, IT["KEY"], "KEY")
        go_door(pb, 0, 1, 1)
        check(not (items(pb) & (1 << IT["KEY"])), "KEY consumed at castle gate")

        # Courtyard: the well is here, the rope is NOT — it waits at
        # the end of the patio/garden/shed chain.
        denied_at(pb, 1, 0, "well without rope")
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
        go_door(pb, 1, 2, 2)

        # Hall -> kitchen (west): MIRROR, then pantry -> larder ->
        # cellar: OIL, all the way back.
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

        # Hall -> library (east): the prism is take-locked without the
        # star chart, so this visit only proves the denial.
        go_door(pb, 2, 1, 4)
        denied_at(pb, 4, 1, "prism without chart")
        check(not (items(pb) & (1 << IT["PRISM"])), "prism still hidden")
        go_door(pb, 4, 2, 2)

        # Hall gate A -> chapel (HUMILITY sacrifice).
        sacrifice(pb, 2, 2, 1)
        check(rd(pb, "gates") & 0x01, "gate A opened")
        go_door(pb, 2, 2, 5)

        # Chapel gate B -> gallery (PATIENCE sacrifice). No horn here
        # any more; it waits in the locked choir loft.
        sacrifice(pb, 5, 3, 2)
        check(rd(pb, "gates") & 0x02, "gate B opened")
        go_door(pb, 5, 3, 7)

        # Gallery: LADDER, then armory: AXE, back.
        take_at(pb, 7, 3, IT["LADDER"], "LADDER")
        go_door(pb, 7, 5, 18)
        take_at(pb, 18, 0, IT["AXE"], "AXE")
        go_door(pb, 18, 1, 7)
        go_door(pb, 7, 4, 5)

        # Chapel stair down consumes LADDER -> crypt.
        go_door(pb, 5, 2, 6)
        check(not (items(pb) & (1 << IT["LADDER"])), "LADDER consumed at descent")

        # Crypt: BELL, coffer (CANDLE consumed, OBSIDIAN gained).
        take_at(pb, 6, 0, IT["BELL"], "BELL")
        use_at(pb, 6, 4)
        check(not (items(pb) & (1 << IT["CANDLE"])), "CANDLE consumed at coffer")
        check(items(pb) & (1 << IT["OBSIDIAN"]), "OBSIDIAN from coffer")
        check(rd(pb, "coffer") == 1, "coffer marked used")
        go_door(pb, 6, 2, 5)

        # Chapel gate (open) -> gallery; glass USE consumes MIRROR.
        go_door(pb, 5, 3, 7)
        use_at(pb, 7, 0)
        check(rd(pb, "mirror") == 1, "mirror used at glass")
        check(not (items(pb) & (1 << IT["MIRROR"])), "MIRROR consumed at glass")
        go_door(pb, 7, 1, 8)

        # Stair: guardian USE consumes BELL, gate -> observatory.
        use_at(pb, 8, 0)
        check(rd(pb, "bell") == 1, "bell used at guardian")
        check(not (items(pb) & (1 << IT["BELL"])), "BELL consumed at guardian")
        go_door(pb, 8, 1, 10)

        # Observatory: chart room holds the STAR CHART. Fetch it, then
        # backtrack all the way to the library for the prism.
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
        go_door(pb, 4, 2, 2)
        go_door(pb, 2, 2, 5)
        go_door(pb, 5, 3, 7)
        go_door(pb, 7, 1, 8)
        go_door(pb, 8, 1, 10)

        # Observatory: lens USE consumes PRISM, yields SUNSTONE; gate -> chasm.
        use_at(pb, 10, 0)
        check(rd(pb, "lens") == 1, "prism used at lens")
        check(not (items(pb) & (1 << IT["PRISM"])), "PRISM consumed at lens")
        check(items(pb) & (1 << IT["SUNSTONE"]), "SUNSTONE from lens")
        go_door(pb, 10, 2, 11)

        # Backtrack to the courtyard: the chasm needs wood, and the only
        # dead tree stands in the very first room.
        go_door(pb, 11, 3, 10)
        go_door(pb, 10, 3, 8)
        go_door(pb, 8, 2, 7)
        go_door(pb, 7, 4, 5)
        go_door(pb, 5, 4, 2)
        go_door(pb, 2, 3, 1)

        # Courtyard tree: AXE consumed, WOOD gained.
        use_at(pb, 1, 1)
        check(not (items(pb) & (1 << IT["AXE"])), "AXE consumed at tree")
        check(items(pb) & (1 << IT["WOOD"]), "WOOD from tree")
        check(rd(pb, "tree") == 1, "tree marked felled")
        go_door(pb, 1, 2, 2)

        # Forward again through the already-opened gates to the chasm.
        go_door(pb, 2, 2, 5)
        go_door(pb, 5, 3, 7)
        go_door(pb, 7, 1, 8)
        go_door(pb, 8, 1, 10)
        go_door(pb, 10, 2, 11)

        # Chasm: wood USE builds bridge; crossing -> archive.
        use_at(pb, 11, 0)
        check(rd(pb, "bridge") == 1, "bridge built by wood")
        check(not (items(pb) & (1 << IT["WOOD"])), "WOOD consumed at chasm")
        go_door(pb, 11, 1, 12)

        # Archive -> smithy: KEY BOW, back.
        go_door(pb, 12, 3, 15)
        take_at(pb, 15, 0, IT["BOW"], "KEY BOW")
        go_door(pb, 15, 2, 12)

        # Archive passage consumes OIL -> echo.
        go_door(pb, 12, 0, 13)
        check(not (items(pb) & (1 << IT["OIL"])), "OIL consumed at passage")

        # Echo: gatekeeper and choir loft both deny without the horn
        # and the loft key. The loft key waits at the end of the long
        # passage chain off this very hall.
        denied_at(pb, 13, 0, "gatekeeper without horn")
        denied_at(pb, 13, 5, "choir loft without loft key")
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

        # Choir loft door consumes the LOFT KEY; the horn waits inside.
        go_door(pb, 13, 5, 23)
        check(not (items(pb) & (1 << IT["LOFTKEY"])),
              "LOFT KEY consumed at choir loft door")
        take_at(pb, 23, 0, IT["HORN"], "HORN")
        go_door(pb, 23, 2, 13)

        # Echo: horn USE wakes gatekeeper; belfry: KEY STEM, back.
        use_at(pb, 13, 0)
        check(rd(pb, "echo") == 1, "horn used at gatekeeper")
        check(not (items(pb) & (1 << IT["HORN"])), "HORN consumed at gatekeeper")
        go_door(pb, 13, 4, 16)
        take_at(pb, 16, 0, IT["STEM"], "KEY STEM")
        go_door(pb, 16, 1, 13)

        # Back to the smithy forge: BOW + STEM + SUNSTONE -> FINAL KEY.
        go_door(pb, 13, 3, 12)
        go_door(pb, 12, 3, 15)
        use_at(pb, 15, 1)
        check(not (items(pb) & (1 << IT["BOW"])), "BOW consumed at forge")
        check(not (items(pb) & (1 << IT["STEM"])), "STEM consumed at forge")
        check(not (items(pb) & (1 << IT["SUNSTONE"])), "SUNSTONE consumed at forge")
        check(items(pb) & (1 << IT["FINALKEY"]), "FINAL KEY from forge")
        go_door(pb, 15, 2, 12)

        # Rusted passage again without OIL (stays open) -> echo gate.
        go_door(pb, 12, 0, 13)
        go_door(pb, 13, 2, 14)

        # Reliquary sanctum consumes OBSIDIAN -> vestibule; exit with FINAL KEY.
        go_door(pb, 14, 0, 9)
        check(not (items(pb) & (1 << IT["OBSIDIAN"])), "OBSIDIAN consumed at sanctum")
        check(items(pb) == (1 << IT["FINALKEY"]),
              f"final inventory is FINAL KEY alone (got {items(pb):#x})")
        goto_hot(pb, 0)
        interact(pb)
        assert wait_state(pb, ST_END), "expected ST_END"
        check(rd(pb, "sins") == 2, "pure ending at 2 sins")
        check(rd(pb, "end_track") == 1, "pure ending track selected")
        pb.screen.image.save(os.path.join(OUT, "05-ending-pure.png"))
        print("PURE RUN", "FAILED" if fails else "PASSED",
              f"({len(fails)} failures)" if fails else "")
        return 1 if fails else 0
    finally:
        pb.stop()

if __name__ == "__main__":
    sys.exit(main())
