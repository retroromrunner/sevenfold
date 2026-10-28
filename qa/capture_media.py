"""SEVENFOLD launch-media capture (ship-prep, final ROM only).

Replays the pure run (2 sins) with PyBoy and captures, from the FINAL
ROM in releases/sevenfold.gb:
  - media/title.png, courtyard.png, crypt.png, bridge.png, ending-pure.png
    (native 160x144, two colors)
  - media/gameplay.gif (3x nearest-neighbor, title -> pickup -> rooms
    -> sacrifice -> bridge -> ending)
  - media/cover.png (title screen, 4x nearest-neighbor = 640x576)
Never reuse old-build screenshots: everything here is captured fresh.
"""
import os, sys
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
sys.path.insert(0, os.path.expanduser("~/workspace/sevenfold/qa"))
sys.path.insert(0, os.path.expanduser(
    "~/workspace/gbc/tools/venv/lib/python3.12/site-packages"))
from PIL import Image
from pyboy import PyBoy
import playthrough as pt

ROM = os.path.expanduser("~/workspace/sevenfold/releases/sevenfold.gb")
OUT = os.path.expanduser("~/workspace/sevenfold/media")
os.makedirs(OUT, exist_ok=True)

ST_TITLE, ST_ROOM, ST_TEXT, ST_SAC, ST_INV, ST_END = 0, 1, 2, 3, 4, 5
tick, press, rd = pt.tick, pt.press, pt.rd
take_at, go_door, use_at = pt.take_at, pt.go_door, pt.use_at
denied_at, sacrifice = pt.denied_at, pt.sacrifice
IT = pt.IT

frames = []


def shot(name):
    pb.screen.image.save(os.path.join(OUT, name))
    print("saved", name)


def gif_frame():
    img = pb.screen.image.resize((480, 432), Image.NEAREST)
    frames.append(img.convert("P", palette=Image.ADAPTIVE, colors=4))


def main():
    global pb
    pb = PyBoy(ROM, window="null")
    pb.set_emulation_speed(0)
    try:
        tick(pb, 90)
        assert rd(pb, "state") == ST_TITLE, "boots to title"
        shot("title.png")
        gif_frame(); tick(pb, 30); gif_frame(); tick(pb, 30); gif_frame()
        press(pb, "start")
        assert pt.wait_state(pb, ST_TEXT), "expected intro text"
        pt.close_text(pb)
        assert rd(pb, "room") == 0

        take_at(pb, 0, 0, IT["KEY"], "KEY")
        gif_frame()
        go_door(pb, 0, 1, 1)
        shot("courtyard.png")
        gif_frame(); gif_frame()
        denied_at(pb, 1, 0, "well without rope")
        go_door(pb, 1, 4, 19)
        gif_frame()
        go_door(pb, 19, 3, 20)
        go_door(pb, 20, 3, 21)
        take_at(pb, 21, 0, IT["ROPE"], "ROPE")
        gif_frame()
        go_door(pb, 21, 2, 20)
        go_door(pb, 20, 2, 19)
        go_door(pb, 19, 2, 1)
        use_at(pb, 1, 0)
        gif_frame()
        go_door(pb, 1, 2, 2)
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
        go_door(pb, 2, 1, 4)
        denied_at(pb, 4, 1, "prism without chart")
        go_door(pb, 4, 2, 2)
        sacrifice(pb, 2, 2, 1)
        gif_frame()
        go_door(pb, 2, 2, 5)
        sacrifice(pb, 5, 3, 2)
        go_door(pb, 5, 3, 7)
        take_at(pb, 7, 3, IT["LADDER"], "LADDER")
        go_door(pb, 7, 5, 18)
        take_at(pb, 18, 0, IT["AXE"], "AXE")
        go_door(pb, 18, 1, 7)
        go_door(pb, 7, 4, 5)
        go_door(pb, 5, 2, 6)
        shot("crypt.png")
        gif_frame()
        take_at(pb, 6, 0, IT["BELL"], "BELL")
        use_at(pb, 6, 4)
        go_door(pb, 6, 2, 5)
        go_door(pb, 5, 3, 7)
        use_at(pb, 7, 0)
        go_door(pb, 7, 1, 8)
        use_at(pb, 8, 0)
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
        go_door(pb, 4, 2, 2)
        go_door(pb, 2, 2, 5)
        go_door(pb, 5, 3, 7)
        go_door(pb, 7, 1, 8)
        go_door(pb, 8, 1, 10)
        use_at(pb, 10, 0)
        go_door(pb, 10, 2, 11)
        go_door(pb, 11, 3, 10)
        go_door(pb, 10, 3, 8)
        go_door(pb, 8, 2, 7)
        go_door(pb, 7, 4, 5)
        go_door(pb, 5, 4, 2)
        go_door(pb, 2, 3, 1)
        use_at(pb, 1, 1)
        go_door(pb, 1, 2, 2)
        go_door(pb, 2, 2, 5)
        go_door(pb, 5, 3, 7)
        go_door(pb, 7, 1, 8)
        go_door(pb, 8, 1, 10)
        go_door(pb, 10, 2, 11)
        use_at(pb, 11, 0)
        shot("bridge.png")
        gif_frame(); gif_frame()
        go_door(pb, 11, 1, 12)
        go_door(pb, 12, 3, 15)
        take_at(pb, 15, 0, IT["BOW"], "KEY BOW")
        go_door(pb, 15, 2, 12)
        go_door(pb, 12, 0, 13)
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
        go_door(pb, 13, 5, 23)
        take_at(pb, 23, 0, IT["HORN"], "HORN")
        go_door(pb, 23, 2, 13)
        use_at(pb, 13, 0)
        go_door(pb, 13, 4, 16)
        take_at(pb, 16, 0, IT["STEM"], "KEY STEM")
        go_door(pb, 16, 1, 13)
        go_door(pb, 13, 3, 12)
        go_door(pb, 12, 3, 15)
        use_at(pb, 15, 1)
        go_door(pb, 15, 2, 12)
        go_door(pb, 12, 0, 13)
        go_door(pb, 13, 2, 14)
        go_door(pb, 14, 0, 9)
        pt.goto_hot(pb, 0)
        pt.interact(pb)
        assert pt.wait_state(pb, ST_END), "expected ST_END"
        assert rd(pb, "sins") == 2, "pure ending at 2 sins"
        shot("ending-pure.png")
        gif_frame(); tick(pb, 30); gif_frame()

        frames[0].save(os.path.join(OUT, "gameplay.gif"), save_all=True,
                       append_images=frames[1:], duration=180, loop=0,
                       optimize=True)
        print(f"saved gameplay.gif ({len(frames)} frames)")
        cover = Image.open(os.path.join(OUT, "title.png")).resize(
            (640, 576), Image.NEAREST)
        cover.save(os.path.join(OUT, "cover.png"))
        print("saved cover.png (640x576)")
        print("CAPTURE", "FAILED" if pt.fails else "PASSED",
              f"({len(pt.fails)} failures)" if pt.fails else "")
        return 1 if pt.fails else 0
    finally:
        pb.stop()


if __name__ == "__main__":
    sys.exit(main())
