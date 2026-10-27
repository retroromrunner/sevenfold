# SEVENFOLD — submit notes (staging only, do NOT submit yet)

- Cori's standing rule: never submit until she playtests and approves the
  exact build.
- Added constraint (Oct 27, 2026, from Cori): do NOT submit the entry
  until at least a few other entries have already been submitted to
  1-BIT JAM 15. She does not want to be the first one in and look AI.
  Check the jam's entry count before any submit step, and only submit
  once the field is non-trivial — and only after her playtest approval.
- Deadline safety rule: aim to submit before noon EST Nov 2 regardless
  of the jam page banner's ambiguous timezone.
- No submission, upload, or jam-page edit happens from this workspace
  until Cori explicitly approves the exact build in chat.

## Oct 27 playtest fixes (Cori: "This is great actually!") — applied, re-QA'd

Build: releases/sevenfold.gb (32KB, md5 cd0b6c53dbf04b9c41037ad1ec2d94ea),
staged wrapper refreshed (itch/sevenfold.gb identical md5). Nothing
published, submitted, or uploaded anywhere.

1. Spooky slow music: a looping slow minor-key melody on sound
   channel 2 (Shadowgate-slow), separate from SFX on channel 1 so
   effects never stomp the tune. Headless QA cannot hear it: register
   proof only (qa/audio_regs.py: sound on, ch2 active, music envelope
   written, routing intact). Music quality is for Cori's ears.
2. Item sprite disappears on pickup: the room map previously redrew
   the item tile forever; draw_room now blanks the tile of any taken
   item. Proven by tilemap reads in qa/item_disappear.py (key tile
   11 -> floor 3 after pickup; same for mirror) plus screenshots
   10-key-gone.png and 11-mirror-gone.png.
3. Every item required on the critical path (Cori: rope etc. felt
   unneeded, because rope+candle only opened the optional crypt):
   - The candle now comes from the courtyard well: USE rope at the
     well to haul up a burning candle (signposted: "SOMETHING HANGS
     WITHIN REACH. ROPE COULD FETCH IT.").
   - The silver bell moved from the library to the crypt, so reaching
     it requires the rope descent plus candle light (crypt bell is
     also darkness-gated: "IT IS PITCH BLACK IN THE CRYPT.").
   - Result: key (castle gate), rope (well + descent), candle
     (descent light + crypt dark), mirror (gallery glass + gate),
     bell (guardian + final gate) are all required to finish. No
     decorative items remain; no new rooms added.
   - All three QA runs were rewritten for the new flow and pass end
     to end on the new ROM: pure (2 sins), tainted (4 sins), keeper
     (7 sins). Endings bands unchanged.
4. Distinct sound effects, table-driven on channel 1: pickup (rising
   two-note), seal/gate unlock (rising arpeggio), sacrifice (low
   descending), door travel (short two-note), denied action (low
   rumble). Denied SFX wired to every locked/missing-requirement
   message. Register proof in qa/audio_regs.py (NR12 0xF0 while
   sounding, 0x00 when done; NR51 routing untouched). SFX quality
   and distinctness are for Cori's ears.
5. Proper title screen: the old small castle art is replaced with a
   big horned skull (devil/skeleton) above SEVENFOLD / PRESS START /
   1-BIT JAM 15. The game boots to this screen and waits for Start;
   it is never skipped (QA asserts ST_TITLE at boot and captures
   01-title.png). Title art is two colors only, like everything else.

Still not verified by automation (needs Cori's playtest): music and
SFX quality, puzzle readability, text fit in context, fun, the
EmulatorJS wrapper in a real browser, real hardware.

## Oct 27 polish: screen wipe transition between rooms (Cori's request)

Build: releases/sevenfold.gb (32KB, md5 8b47eff6391aa9ddbe93f2e7018d8183),
staged wrapper refreshed (itch/sevenfold.gb identical md5), playtest
link republished to serve the new build
(https://opaque-quill-b83x.here.now/). Nothing published, submitted,
or uploaded anywhere else.

- What it looks like: when the player moves to a new room, the new
  room reveals column by column, left to right, instead of an instant
  cut. One column per vblank, ~20 frames (~0.33s), so it feels snappy.
  Cursor sprite is hidden during the wipe; the final frame is
  identical to the old instant draw.
- How it stays safe: each column is written during vblank (no
  tearing); the joypad is not polled mid-wipe, so no input is accepted
  until the new room is fully revealed; music_tick/sfx_tick run once
  per wipe frame, so music and SFX continue uninterrupted. Applied to
  every enter_room call (doors, open gates, title -> first room);
  returning from text/sacrifice/inventory screens still redraws
  instantly, since those are not room moves.
- QA: all prior scripts re-run and pass on the new ROM (pure 2 sins,
  keeper 7 sins, tainted 4 sins, item-disappear, audio registers).
  New qa/wipe.py proves the wipe from the VRAM tilemap: it catches a
  mid-wipe frame whose changed columns form one contiguous run
  ([3,4,5], columns 0-2 being identical between the two rooms) that
  extends further right five frames later ([3..10]), plus mid-wipe
  screenshot 12-wipe-mid.png and post-wipe 13-courtyard-after-wipe.png.
- Still not verified by automation: how the wipe feels on real
  hardware or in the browser wrapper (frame pacing there is the
  emulator's), and whether 0.33s feels right to Cori's eye.

## Oct 27 round-2 playtest fixes (Cori: "Pretty good!") — applied, re-QA'd

Build: releases/sevenfold.gb (32KB, md5 f760483f9b10f965828d10cc46cf1ebb),
staged wrapper refreshed (itch/sevenfold.gb identical md5), playtest
link republished to serve the new build
(https://opaque-quill-b83x.here.now/). Nothing published, submitted,
or uploaded anywhere else.

1. Music title-screen-only, new style: the background tune now plays
   ONLY on the title screen and stops dead when the game starts
   (mus_on flag; NR22 silenced at game start, restarted on return to
   title). Rewritten as a slow arpeggio in spooky A harmonic minor
   (A2 C3 E3 A3 G#3 E3 C3, then F3 E3 D3 C3 B2 A2, ~20s loop). SFX
   still work in-game on channel 1. Proven by register: NR22 reads
   0x70 on title and 0x00 in-room (qa/audio_regs.py). Melody quality
   is for Cori's ears (freq registers are write-only headless).
2. Castle expanded with 5 new rooms (15 total), each gated on an
   item, integrated between the stair guardian and the vestibule —
   no existing rooms or puzzles removed:
   - Observatory: USE the mirror at the star lens to open the lens
     gate (new g_lens_used flag).
   - Chasm: door crossing requires the rope (new LK_ROPE lock).
   - Archive: dark passage requires the candle (new LK_CANDLE lock).
   - Echo Hall: USE the bell to wake the sleeping gatekeeper and
     open the echo gate (new g_echo_used flag).
   - Reliquary: sanctum door requires the key (LK_KEY) into the
     vestibule.
   All 5 items stay required on the critical path (the new gates
   re-require them); all three endings still reachable with the
   unchanged 2-required-gates + 5-optional-pacts structure. All
   three QA playthroughs rewritten for the new wing and pass end
   to end (pure 2 sins, tainted 4 sins, keeper 7 sins).
3. Pointer sprite closer to its item: root cause found — GBDK's
   move_sprite takes screen coords MINUS 8 (x) and MINUS 16 (y), so
   passing raw tile pixels rendered the diamond a full tile left
   and two tiles up from the hotspot. New place_cursor() puts it
   immediately left of the hotspot tile, same row, edge touching
   edge. Proven by OAM reads in qa/pointer.py (screen (32,16) for
   the gatehouse key tile at (5,2)) plus screenshot 14-pointer.png.
4. Screen-transition sound: the two-short-tones move sound is
   replaced by a longer rising harmonic-minor scale run (~22
   frames, matching the ~20-frame wipe), played from enter_room so
   it runs across every room change. Distinct from pickup, unlock,
   sacrifice, and denied SFX. Proven sounding mid-wipe by register
   (NR12=0xF0) in qa/audio_regs.py; how it feels is for Cori's ears.
- Still not verified by automation: music and transition-sound
  quality, new-puzzle difficulty and readability, text fit in the
  new rooms, fun, the EmulatorJS wrapper in a real browser, real
  hardware.


## Design deviation from DESIGN.md (builder's call, Oct 27 — needs Cori's eye)

DESIGN.md specified ~5 required sacrifice gates AND endings at 0-2 sins
(pure) / 3-6 (tainted) / 7 (keeper). Those two specs contradict each
other: 5 forced sacrifices make the pure ending unreachable (minimum
would be 5 sins). The build resolves it as follows, keeping every
ending reachable and sacrifices central:

- Required sacrifice gates: 2 (Great Hall -> Chapel, Chapel -> Gallery).
- The other gates open via items, not sins: Gallery gate via the mirror,
  Stair gate via the bell, Final gate via the assembled FINAL KEY
  (round 4; was the sunstone alone in rounds 2-3).
- Optional sacrifices: 5 (portrait pact, chapel altar, crypt skull,
  gallery shrine, throne kneel). 2 required + 5 optional = 7 max, so
  pure (2 sins, no optionals), tainted (3-6), and keeper (7) are all
  reachable, and the pact softlock guard (optionals blocked when
  remaining virtues would fall below remaining required gates) still
  applies. No mechanical effects from sins, per the brief.

## Oct 27 round-3 playtest fixes (Cori: "Very good!") — applied, re-QA'd

Build: releases/sevenfold.gb (32KB, md5 869216a8e24cd1f3f80dbed2fa9e1ed2),
staged wrapper refreshed (itch/sevenfold.gb identical md5), playtest
link republished to serve the new build
(https://opaque-quill-b83x.here.now/). Nothing published, submitted,
or uploaded anywhere else.

1. Single-use items, no reuse: the economy is rebuilt around 12
   items (g_items widened to uint16_t), each consumed at exactly one
   obstacle — KEY castle gate, ROPE well, CANDLE coffer, MIRROR
   glass, BELL guardian, LADDER descent, AXE chasm tree, PRISM lens,
   OIL archive passage, HORN gatekeeper, OBSIDIAN sanctum, SUNSTONE
   final exit. Door locks that check an item consume it on passing;
   USE hotspots are bound to one item each, so an item cannot be
   applied to the wrong obstacle, and a denied attempt consumes
   nothing. Proven by the three rewritten playthroughs (every
   consumption asserted bit-by-bit; final inventory is SUNSTONE
   alone) and by new qa/single_use.py (denied gate and denied well
   leave the inventory unchanged, items still takeable after).
2. Pickup feedback order: the take handler now blanks the item's
   room tile BEFORE show_message runs, so the sprite is already gone
   while the pickup message is on screen. Proven by rewritten
   qa/item_disappear.py, which reads the VRAM tilemap during ST_TEXT
   (key tile 11 -> floor 3, mirror tile 13 -> floor 3) plus
   screenshots 10-key-gone.png and 11-mirror-gone.png taken mid-message.
3. Spatial separation: oil (kitchen) vs archive passage, prism
   (library) vs observatory lens, horn (chapel) vs echo gatekeeper,
   axe (crypt) vs chasm tree, candle (courtyard well) vs crypt
   coffer — no more pickup-and-use in the same room for these.
   Same 15 rooms, no additions; placement rearranged only.
4. Backtracking chains (3): gallery ladder -> chapel descent;
   courtyard-well candle -> crypt coffer -> obsidian key ->
   reliquary sanctum; library prism -> observatory lens -> sunstone
   -> final exit. Plus the existing loop (crypt bell -> stair
   guardian). All signposted by denied/USE hint text.
- QA note (harness quirk, not a game bug): back-to-back same-button
  PyBoy presses with only 3 release ticks can merge — the release
  edge is not seen before the next press, so a sacrifice-menu
  confirm right after the gate-opening press was swallowed. The run
  scripts now release 10 ticks and settle 10 ticks after the menu
  opens; all three runs pass deterministically. A human pressing A
  twice with the menu visible between presses never hits this.
- Still not verified by automation: music and transition-sound
  quality, new-puzzle difficulty and readability, text fit in the
  new rooms, fun, the EmulatorJS wrapper in a real browser, real
  hardware.

## Oct 27 round-4 playtest fixes (Cori's bug + 3 design upgrades) — applied, re-QA'd

Build: releases/sevenfold.gb (32KB, ROM ONLY, md5
0e68af24f6a813336cf7a86df6818831), staged wrapper refreshed
(itch/sevenfold.gb identical md5), playtest link republished to
serve the new build (https://opaque-quill-b83x.here.now/). Nothing
published, submitted, or uploaded anywhere else.

1. BUG FIX — used items respawn: after taking the ladder and using
   it, returning to the gallery showed the ladder takeable again
   (same for the mirror). Root cause confirmed in code: room art
   and takeability keyed off inventory bits (g_items), which clear
   on consumption — so consuming an item un-took it. Fix: a new
   persistent g_taken bitmask (WRAM 0xC0C4, set on TAKE, never
   cleared) now drives draw_room/draw_room_col blanking and
   hs_active takeability; inventory possession is separate.
   Related persistent-unlock fix: a new g_locks_open bitmask (WRAM
   0xC0C6) records every item-locked door already passed, so the
   ladder descent (and castle gate, oil passage, sanctum door) stay
   open on revisit without the consumed item. Flag locks (mirror,
   bell, lens, echo, bridge) already persisted via used-flags, and
   the coffer got its own g_coffer_used flag (it previously
   re-checked the obsidian bit, so it could re-trigger text).
   Proven by new qa/no_respawn.py (53 checks): key/mirror/ladder
   tiles read as floor on revisit, their hotspots never
   cursor-reachable across 10+ RIGHT cycles, g_taken bits survive
   consumption, the second ladder descent reaches the crypt with no
   ladder, and the castle gate re-opens without the key.
   (One script-side red herring during QA: the lock constants in
   the first no_respawn draft were wrong — LK_KEY=1, LK_LADDER=9,
   not 0/1 — and locks must be read as 16 bits; the game was right,
   the script was fixed.)
2. Chasm made harder with a long backtrack: the dead tree moved
   from the chasm to the very first room (courtyard, tile (16,3)).
   The axe moved to the new armory (off the gallery). Route: reach
   the chasm, backtrack all the way to the courtyard, USE the axe
   at the tree (axe consumed, WOOD gained, g_tree_felled set), carry
   the wood back to the chasm, USE wood to build the bridge. The
   chasm denied message and the chasm LOOK text now point at the
   courtyard tree ("THE CHASM NEEDS WOOD. A DEAD TREE STANDS IN THE
   COURTYARD."). Single-use preserved: axe once at the tree, wood
   once at the chasm.
3. Final gate needs a challenging unlock: the sunstone-alone exit is
   replaced by a THREE-PIECE final key. New items: KEY BOW (smithy),
   KEY STEM (belfry), and the SUNSTONE (still from the observatory
   lens). The three are joined at a forge USE hotspot in the smithy
   (denied text lists the missing pieces) into the FINAL KEY, which
   alone opens the exit. The forge run forces a full late-game loop
   (chasm -> archive -> smithy -> echo -> belfry -> echo -> archive
   -> smithy forge -> archive -> echo -> reliquary -> vestibule).
4. Max ONE collectible item per room: audited all 19 rooms; the
   round-3 violations (kitchen held MIRROR + OIL, crypt held BELL +
   AXE) are fixed by the four new rooms — pantry (OIL, off the
   kitchen), armory (AXE, off the gallery), smithy (BOW, off the
   archive, also home of the forge), belfry (STEM, off the echo).
   castle. Rooms with no collectibles (hall, stair,
   vestibule, reliquary, observatory, chasm, archive,
   echo) are intentional breathing room — big castle.
   castle. Full round-4 map/item list:
   - 0 gatehouse: KEY. 1 courtyard: ROPE (+ well, dead tree).
     2 hall: none. 3 kitchen: MIRROR. 4 library: PRISM.
     5 chapel: HORN. 6 crypt: BELL (+ coffer). 7 gallery: LADDER.
     8 stair: none. 9 vestibule: none (exit). 10 observatory: none
     (+ lens). 11 chasm: none (+ bridge gap). 12 archive: none.
     13 echo: none. 14 reliquary: none. 15 smithy: KEY BOW (+ forge).
     16 belfry: KEY STEM. 17 pantry: OIL. 18 armory: AXE.
   16 items, each used exactly once; final inventory in all three
   runs is FINAL KEY alone (0x8000).
- QA: all three playthroughs rewritten for the new economy/map and
  pass end to end (pure 2 sins, keeper 7 sins, tainted 4 sins), plus
  the full suite on the new ROM: no_respawn (new, 53 checks),
  item-disappear (courtyard tree hotspot added to the route),
  single-use, wipe, pointer, audio registers, portrait probe.
  Existing WRAM addresses unchanged (new variables appended after
  g_well_used: g_taken 0xC0C4, g_locks_open 0xC0C6, g_coffer_used
  0xC0C8, g_tree_felled 0xC0C9). Tile indices for KEY/MIRROR/FLOOR
  unchanged (new T_BOW/T_STEM appended after T_TREE). The scripts
  now navigate with a goto_hot helper (RIGHT until the cursor
  matches) instead of counted presses, so they are robust to cursor
  landing differences.
- Still not verified by automation: music and transition-sound
  quality, new-puzzle difficulty and readability (especially the
  long chasm backtrack and the forge hunt — do the hints read
  clearly?), text fit in the new rooms, fun, the EmulatorJS wrapper
  in a real browser, real hardware. Those need Cori's playtest.

## Oct 27 round-5 playtest fixes (Cori) — applied, re-QA'd

Build: releases/sevenfold.gb (32KB, md5 0ea720437b067789f269c714cf04b8fe),
staged wrapper refreshed (itch/sevenfold.gb identical md5), playtest
link republished to serve the new build
(https://opaque-quill-b83x.here.now/). Nothing published, submitted,
or uploaded anywhere else. 31 rooms, 18 items, still 32KB ROM ONLY.

1. Visible bridge: after USEing wood at the chasm, a log now lies
   across the gap row (new T_BRIDGE tile, drawn in draw_room and in
   the wipe column draw, keyed off the persistent g_bridge_built).
   Proven by new qa/bridge.py: the gap row reads all T_DARK before
   the USE, all T_BRIDGE (cols 1-18, walls untouched) after, and
   still all T_BRIDGE after leaving and re-entering through the wipe
   (screenshot 15-bridge.png).
2. Per-ending music on the final screen: do_ending now starts a
   looping channel-2 track matched to the ending — PURE: ascending
   C-major run resolving home; TAINTED: A-minor line ending on the
   dominant E, unresolved; KEEPER: harmonic-minor line sinking step
   by step to a long low A. The music engine gained a sequence
   pointer (title tune still restarts on return to title; game start
   still silences). Proven by register + state: qa/audio_regs.py
   pokes each sin band to the final gate and asserts NR22=0x70
   (channel 2 live) and g_end_track 1/2/3 respectively. Melody
   quality and per-ending feel are for Cori's ears.
3. Well/rope order REVERSED per Cori's correction: the well stays
   early in the courtyard; the ROPE moves to a shed at the end of a
   new patio -> garden -> shed chain (all three rooms carry creepy
   look-only scenery: cracked statue, dead roses, stone angel,
   overgrown hedge, rusted tools). Flow: meet the well, denied
   without rope (denied text now names the patio), fetch the rope,
   backtrack to the well for the candle.
4. Prism gated by a new item: the library prism take is now locked
   behind the STAR CHART (LK_CHART take lock: denied without it,
   chart consumed on the take, single use). The chart lives in a new
   chart room off the observatory (celestial globe look). Flow:
   reach the observatory, find the chart, backtrack to the library
   for the prism, back to the lens. Prism denial is covered in
   qa/single_use.py; chart respawn is covered in qa/no_respawn.py.
5. Horn behind a long side passage: the horn moved from the chapel
   to a locked choir loft off the echo hall (LK_HORNKEY door lock,
   consumes the LOFT KEY, stays open after). The LOFT KEY waits in a
   deep cell at the end of a 5-room passage chain off the echo hall
   (passage I-IV + deep cell, each with creepy look text). Full
   chain: echo -> passage chain -> loft key -> back -> choir loft
   -> horn -> back -> gatekeeper. Every step signposted (gatekeeper
   denial names the loft, loft denial names the passage, loft-key
   take names the loft, horn take names the gatekeeper).
6. Oil likewise: the flask moved from the pantry to a cellar at the
   end of a new pantry -> larder -> cellar chain (empty shelves and
   dead herbs looks signpost the way down). Single use at the
   archive passage unchanged.
- Implementation notes: items 16 (CHART) and 17 (LOFTKEY) live in a
  second bitmask pair (g_items2 0xC0CA, g_taken2 0xC0CC, g_end_track
  0xC0CE, all appended so every existing WRAM address is unchanged);
  all arbitrary-item-id code goes through item_has/item_give/
  item_drop/taken_has/taken_set helpers. New qa/map_check.py parses
  gen_tiles.py maps against the main.c hotspot tables (291 checks:
  take-on-item-tile, door-on-door-tile, <=1 collectible per room,
  valid door targets) — it caught a real off-by-one in the echo
  map's new doors before the first QA run. Inventory screen
  relaid out for 18 items (portrait top-left, two columns of 9,
  7-row virtue/sin lists, nothing clips). Ending screenshots and
  the bridge shot verify exactly 2 colors on screen.
- QA: all three playthroughs rewritten for the new map and pass end
  to end (pure 2 sins, tainted 4 sins, keeper 7 sins, final inventory
  FINAL KEY alone), plus the full suite on the new ROM: no_respawn
  (extended: rope, chart, oil, loft key, horn revisits; key/mirror/
  ladder/stairs checks kept), item-disappear (new patio-chain
  route), single-use (prism denial added), wipe, pointer, audio
  registers (ending tracks added), portrait probe, map_check (new),
  bridge (new).
- Still not verified by automation: music and per-ending-track
  quality and feel, puzzle readability on the long backtracks
  (especially the 5-room passage chain and the forge hunt — do the
  hints read clearly?), text fit in the new rooms, fun, the
  EmulatorJS wrapper in a real browser, real hardware. Those need
  Cori's playtest.
