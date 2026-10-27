# SEVENFOLD — 1-BIT JAM 15 entry (locked design, Oct 27, 2026)

## Jam constraints (non-negotiable)
- Jam: 1-BIT JAM 15, https://itch.io/jam/1-bit-jam-15. Theme: "Everything Has a Price".
- Game must be made during the jam (Oct 26–Nov 2). Brand-new game; do NOT reuse DREADHOLLOW GBC or any other game's code, rooms, or art. Toolchain, build scripts, and generic know-how may be reused; all game content must be original to this jam.
- TWO COLORS ONLY, everywhere, no exceptions: use exactly two Game Boy shades for all art, sprites, and UI (pick the lightest + darkest GB shades and use nothing else). This is the jam's core rule ("any two colors").
- Platform: Game Boy ROM (.gb), playable in a browser on itch via an EmulatorJS wrapper zip (ROM download also offered). Must also run on real hardware/emulators.
- Deadline discipline: build is TINY and must be submittable before GoedWare kickoff Fri Oct 30, 6:00 AM EDT. Aim for a finished, Designer-playtested build by Oct 29. Do NOT submit anything to the jam until the Designer playtests and approves the exact build (standing rule) AND at least a few other entries have been submitted (check the jam's entry count before any submission).

## Concept (Designer-locked)
Mini Shadowgate-style first-person castle crawler. You are a pilgrim who enters a cursed castle to reach its exit. To advance deeper you must sacrifice virtues at altars/gates. Each sacrifice makes you more sinful and the castle more twisted — but costs you nothing mechanically.

- Title: SEVENFOLD. Boots to a title screen with a big horned-skull
  devil/skeleton above the title; press Start to begin (never skipped).
- View: first-person static rooms (Shadowgate-like). Each room is a full-screen 2-color illustration with hotspots.
- Controls: D-pad cycles hotspots in the room; A interacts (Look/Take/Use via a small verb menu or context action); B backs out/cancels; Start opens inventory + virtues list. On the browser emulator, clicking/tapping a hotspot must do the same as A. Keep controls to this set.
- Inventory puzzles: 18 items, EVERY one required on the critical
  path, EACH USED EXACTLY ONCE (round-3 fix, Oct 27 — Designer found
  item reuse boring; round-4 keeps this): KEY (castle gate), ROPE
  (courtyard well, yields the candle), CANDLE (crypt coffer, yields
  the obsidian key), MIRROR (gallery glass), BELL (stair guardian),
  LADDER (chapel descent), AXE (courtyard dead tree, yields wood),
  WOOD (chasm bridge), STAR CHART (library prism take, consumed),
  PRISM (observatory lens, yields the sunstone), OIL (archive rusted
  passage door), LOFT KEY (choir loft door, consumed), HORN (echo
  gatekeeper), OBSIDIAN (reliquary sanctum door), KEY BOW + KEY STEM
  + SUNSTONE (smithy forge, joined into the final key), FINAL KEY
  (final exit). An item's USE only succeeds at its intended
  obstacle; using it elsewhere shows the denied message and does NOT
  consume it. Door locks that check an item CONSUME it on passing.
  Every puzzle is signposted by in-game text hints.
- Round-5 rules (Oct 27): the well stays early on the regular path
  (courtyard) while the ROPE moves to a shed at the end of a new
  patio -> garden -> shed chain; the prism take is locked behind a
  STAR CHART found in a new chart room off the observatory (backtrack
  to the library); the horn moves to a locked choir loft off the echo
  hall, opened by a LOFT KEY at the end of a 5-room passage chain off
  the echo hall (backtrack to the loft, then back to the gatekeeper);
  the oil moves to a cellar at the end of a new pantry -> larder ->
  cellar chain. The castle grows to 31 rooms, still max ONE
  collectible per room; new rooms carry creepy look-only scenery.
  Items 16-17 live in a second bitmask pair (g_items2/g_taken2) so no
  existing WRAM address shifts. The chasm bridge now draws as a log
  across the gap row once built. The final screen plays its own
  music per ending (pure: ascending major, tainted: minor ending
  unresolved on the dominant, keeper: harmonic-minor descending).
- Round-4 rules (Oct 27): max ONE collectible item per room — the
  castle grew to 19 rooms to spread items out (new: smithy, belfry,
  pantry, armory); rooms with no items are fine. Taken/used state
  persists separately from inventory possession (g_taken bitmask,
  never cleared; g_locks_open bitmask for opened item-lock doors),
  so a consumed item can never respawn in its room and an unlocked
  passage stays open on revisit (notably the ladder descent).
- Round-4 chasm (Oct 27): the dead tree moved to the very first room
  (courtyard). The player finds the axe later, reaches the chasm,
  and must backtrack to the courtyard to fell the tree for wood,
  then carry the wood back to bridge the chasm. Axe used once at
  the tree, wood used once at the chasm; the chasm denied message
  points at the courtyard tree.
- Round-4 final gate (Oct 27): the simple sunstone exit is replaced
  by a three-piece final key. The bow is in the smithy, the stem in
  the belfry, the sunstone still from the observatory lens; the
  three are joined at the smithy forge into the FINAL KEY, which
  alone opens the exit. Pieces are spread far apart; backtracking
  is expected.
- Round-2 expansion (Oct 27): 5 new rooms extend the tower wing
  between the stair guardian and the vestibule, each gated on a
  distinct item so the game is more challenging (round-3 rework:
  the wing gates no longer re-require the early items — Observatory
  lens takes the PRISM, Chasm crossing takes the AXE-felled bridge,
  Archive passage takes OIL, Echo gate takes the HORN, Reliquary
  sanctum takes the OBSIDIAN). 15 rooms total.
- Round-3 spatial separation + backtracking (Oct 27): items sit
  several rooms from their obstacle — the oil is in the pantry but
  oils a door in the archive wing; the prism is in the library but
  feeds the observatory lens; the horn is in the chapel but wakes
  the echo gatekeeper. Backtracking chains: the ladder found in the
  gallery must be carried back to the chapel descent; the candle
  from the courtyard well must be carried to the crypt coffer for
  the obsidian key; the axe found in the armory must be carried
  back to the courtyard tree for wood, then the wood carried to the
  chasm; the bow, stem, and sunstone must be carried to the smithy
  forge for the final key.
- Pickup feedback (round-3 fix): an item's sprite is hidden BEFORE
  its pickup message displays (the take handler blanks the tile,
  then shows the message).
- Virtue/sin system (the theme):
  - Seven virtues, each paired with its sin: Humility→Pride, Patience→Wrath, Charity→Greed, Temperance→Gluttony, Chastity→Lust, Diligence→Sloth, Kindness→Envy.
  - At sacrifice gates the player chooses which virtue to offer. Sacrificing a virtue grants its sin.
  - Sins are VISUAL AND STORY ONLY — no mechanical effects (Designer's deadline call). Do not implement stat changes, trap changes, or difficulty changes from sins.
  - Visual corruption: the player/portrait sprite darkens with each sin gained, and room art gains twisted variants at higher corruption (e.g. alternate tiles/overlays once corruption reaches 4+). Implement this with tile swaps/overlays, cheaply.
  - Gates: 2 required sacrifice gates (Great Hall -> Chapel, Chapel
    -> Gallery) plus 5 optional pacts (portrait, altar, skull,
    shrine, throne), so full corruption is reachable but not forced.
    (The brief's original "~5 required gates" contradicted the
    0-2-sin pure ending; the build resolves it as 2 + 5 = 7 max.)
- Endings (at the castle exit):
  - 0–2 sins: pure ending (escape, mostly untainted).
  - 3–6 sins: tainted ending (escape, changed).
  - All 7 sins: DARK ENDING — you do not escape; you become the castle's new keeper. This is an ending, not a game over.
- Scope: 31 rooms (19 round-4 + 12 round-5: patio, garden, shed,
  chart room, choir loft, four passage rooms, deep cell, larder,
  cellar).
  Linear-ish with small branches; every room reachable, every puzzle
  solvable, every ending reachable. Text must be complete and
  proofread in-game (no placeholder text).
- Audio: TITLE SCREEN ONLY — a slow arpeggio in spooky A harmonic
  minor on sound channel 2 (round-2 fix: in-game music annoyed the Designer;
  it stops dead when the game starts). Distinct table-driven SFX on
  channel 1 (pickup, unlock, sacrifice, denied, and a rising
  harmonic-minor scale run across the room-change wipe). Round-5:
  the FINAL SCREEN plays its own looping channel-2 track per ending
  (pure ascending major, tainted minor unresolved, keeper
  harmonic-minor descending). Headless QA asserts register activity
  and track selection only; music/SFX quality is judged by Designer's
  ears.
- Cursor: the hotspot pointer sits immediately left of its tile,
  edge touching edge (GBDK move_sprite coords are screen-minus-8/16,
  which previously floated the diamond a tile left and two up).

## Tech
- GBDK-2020 C, toolchain at ~/workspace/gbc/tools/gbdk (see ~/workspace/dreadhollow-gbc/build.sh for the lcc invocation pattern: `$GBDK/bin/lcc -Wm-yc ... -Wl-m -o releases/<name>.gb src/main.c`). Plain 32KB ROM is fine; no mapper needed unless size demands it.
- GBDK/SDCC traps (from workspace lessons): never divide by a variable that can be zero (SDCC __divuint hangs on zero divisor) — divide only by compile-time constants or use shifts. Keep hot loops 8/16-bit.
- QA harness: PyBoy headless QA (see ~/workspace/dreadhollow-gbc/qa/ for the pattern: drive inputs, screenshot, read WRAM). A scripted playthrough must reach at least one ending, and screenshots must show: title, a room, inventory/virtues screen, a sacrifice moment, and an ending.
- Deliverables in ~/workspace/sevenfold/: src/, releases/sevenfold.gb, qa/ (script + screenshots), and an EmulatorJS browser wrapper (index.html + emulator + ROM) staged for itch. Do NOT publish to itch, GitHub, or the jam — staging only, until Designer approves.

## Standing rules for this build
- Designer is the final QA: the Designer's playtest verdict gates any push/submit. In-house QA is a smoke test only; report honestly what automated testing did not verify.
- Copy is player-focused, no version numbers/changelogs on public pages.
- Keep replies/status brief; do not pad with technical detail.
