#!/bin/bash
# Build SEVENFOLD. Produces releases/sevenfold.gb (plain 32KB ROM).
# MBC5 was tried for the 1.1 backtrack locks and REVERTED: GBDK
# non-banked code still has to fit the 32KB address window, so the
# MBC5 build stalled at the title-to-game transition in PyBoy QA
# (code overflowed past 0x8000). The locks therefore fit by trimming
# message text instead; see the trim notes in src/main.c comments.
set -e
cd "$(dirname "$0")"
python3 gen_tiles.py
GBDK="$HOME/workspace/gbc/tools/gbdk"
mkdir -p releases
"$GBDK/bin/lcc" -Wm-yn"SEVENFOLD" -Wl-m -o releases/sevenfold.gb src/main.c
ls -la releases/
python3 - <<'EOF'
import sys
with open("releases/sevenfold.gb", "rb") as f:
    d = f.read()
print("size:", len(d), "bytes; header cart type: 0x%02X" % d[0x147])
assert len(d) == 32768, "expected plain 32KB ROM"
assert d[0x147] == 0x00, "expected ROM ONLY cartridge type"
print("header OK: ROM ONLY, 32KB")
EOF
