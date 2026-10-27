#!/bin/bash
# Build SEVENFOLD. Produces releases/sevenfold.gb (plain 32KB ROM).
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
