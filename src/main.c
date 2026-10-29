/* SEVENFOLD - 1-BIT JAM 15 entry.
 * Mini Shadowgate-style first-person castle crawler for Game Boy.
 * Original content made for this jam. TWO COLORS ONLY: all art uses
 * only GB shades 0 (white) and 3 (black). Sins are visual/story only.
 */
#include <gb/gb.h>
#include <gb/hardware.h>
#include "tiles.h"
#include "rooms.h"

/* states */
#define ST_TITLE 0
#define ST_ROOM  1
#define ST_TEXT  2
#define ST_SAC   3
#define ST_INV   4
#define ST_END   5

/* hotspot types */
#define HS_LOOK 0
#define HS_TAKE 1
#define HS_DOOR 2
#define HS_GATE 3
#define HS_PACT 4
#define HS_USE  5
#define HS_EXIT 6

/* door locks: flag locks open via a USE elsewhere (the item is consumed
   at the USE); item locks check one inventory item and CONSUME it on
   passing, so every item is used exactly once, at exactly one
   obstacle. No item gates two obstacles. */
#define LK_NONE 0
#define LK_KEY  1
#define LK_MIRROR 3
#define LK_BELL 4
#define LK_LENS 7
#define LK_ECHO 8
#define LK_LADDER 9
#define LK_OIL 10
#define LK_OBSIDIAN 11
#define LK_BRIDGE 12
#define LK_HORNKEY 13
#define LK_ICEKEY 15
#define LK_GEM 16
#define LK_RIDDLE 17
#define LK_KNIGHT 18
/* LK_CHART is a TAKE lock, not a door lock: the library prism can only
   be taken while carrying the star chart (consumed on the take).
   LK_STARKEY / LK_DROWNEDKEY are the same pattern for the 1.1
   backtrack locks: the Observatory Moonstone needs the Star Key
   (Sunken Chapel), the Pantry Blade needs the Drowned Key (Slot
   Hall), each consumed on its take. */
#define LK_CHART 14
#define LK_STARKEY 19
#define LK_DROWNEDKEY 20

/* gate requirements */
#define RQ_NONE   0
#define RQ_MIRROR 1
#define RQ_BELL   2

/* use kinds: each item has exactly one USE hotspot in the whole game,
   so an item literally cannot be applied to the wrong obstacle. A USE
   that lacks its item shows the denied message and consumes nothing. */
#define USE_MIRROR 0
#define USE_BELL   1
#define USE_WELL   2
#define USE_LENS   3
#define USE_ECHO   4
#define USE_TREE   5
#define USE_COFFER 6
#define USE_WOOD   7
#define USE_FORGE  8
#define USE_POOL   9
#define USE_GEMSLOT 10
#define USE_RIDDLE 11
#define USE_KNIGHT 12

/* items (18): every one is required on the critical path, each consumed
   at exactly one obstacle. Item/obstacle mapping:
   KEY->castle gate, ROPE->courtyard well (yields CANDLE), CANDLE->crypt
   coffer (yields OBSIDIAN), MIRROR->gallery glass, BELL->stair
   guardian, LADDER->chapel descent, AXE->courtyard dead tree (yields
   WOOD), WOOD->chasm bridge, CHART->library prism take (consumed),
   PRISM->observatory lens (yields SUNSTONE), OIL->archive passage,
   LOFTKEY->choir loft door (consumed), HORN->echo gatekeeper,
   OBSIDIAN->sanctum door, BOW+STEM+SUNSTONE->smithy forge (yields
   FINAL KEY), FINAL KEY->final exit. */
#define IT_KEY    0
#define IT_ROPE   1
#define IT_CANDLE 2
#define IT_MIRROR 3
#define IT_BELL   4
#define IT_LADDER 5
#define IT_AXE    6
#define IT_PRISM  7
#define IT_OIL    8
#define IT_HORN   9
#define IT_OBSIDIAN 10
#define IT_SUNSTONE 11
#define IT_WOOD   12
#define IT_BOW    13
#define IT_STEM   14
#define IT_FINALKEY 15
#define IT_CHART  16
#define IT_LOFTKEY 17
/* 1.1 Drowned Wing items (18-25, in g_items2 bits 2-9): MOONSTONE is
   found in the Observatory, BLADE in the Pantry, TORCH in the Torch
   Vault, GEM in the Gem Gallery, SEAL in the Seal Chamber; ICEKEY is
   yielded by melting the frozen pool with the torch. STARKEY is
   found in the Sunken Chapel and unlocks the Moonstone take;
   DROWNEDKEY is found in the Slot Hall and unlocks the Blade take.
   Each is used exactly once, like every other item. */
#define IT_MOONSTONE 18
#define IT_TORCH  19
#define IT_ICEKEY 20
#define IT_GEM    21
#define IT_BLADE  22
#define IT_SEAL   23
#define IT_STARKEY 24
#define IT_DROWNEDKEY 25

/* WRAM state (non-static so the linker map exposes them for PyBoy QA).
 * g_taken is the ROUND-4 bug fix: it records every item ever taken
 * and is NEVER cleared, so a consumed item can never respawn in its
 * room (the old code keyed room art off g_items, whose bits clear on
 * consumption). g_locks_open records every item-locked door already
 * passed, so unlocked passages stay open on revisit without the item
 * (notably the ladder descent). New variables are appended after the
 * old ones so existing WRAM addresses do not shift. */
uint8_t g_state, g_room, g_hot, g_sins, g_gates, g_pacts;
uint16_t g_items;
uint8_t g_mirror_used, g_bell_used, g_virtues, g_pending_id, g_pending_is_pact;
uint8_t g_sac_sel, g_frame, g_lens_used, g_echo_used, g_bridge_built;
uint8_t g_well_used;
uint16_t g_taken, g_locks_open;
uint8_t g_coffer_used, g_tree_felled;
/* Round-5: items 16-17 live in a second bitmask pair, appended here so
   every existing WRAM address stays put. g_end_track records which
   ending track do_ending selected (0 none, 1 pure, 2 tainted,
   3 keeper) so headless QA can prove the right track per sin band. */
uint16_t g_items2, g_taken2;
uint8_t g_end_track;
/* 1.1 Drowned Wing flags, appended so existing WRAM addresses do not shift. */
uint8_t g_pool_melted, g_gem_used, g_riddle_used, g_knight_used;

/* Round-5 item helpers: items 0-15 live in g_items/g_taken, items 16+
   in g_items2/g_taken2. Everything that touches an arbitrary item id
   goes through these so the second word can never be missed. */
static uint8_t item_has(uint8_t it) {
    if (it < 16) return (g_items & (uint16_t)(1u << it)) != 0;
    return (g_items2 & (uint16_t)(1u << (it - 16))) != 0;
}
static void item_give(uint8_t it) {
    if (it < 16) g_items |= (uint16_t)(1u << it);
    else g_items2 |= (uint16_t)(1u << (it - 16));
}
static void item_drop(uint8_t it) {
    if (it < 16) g_items &= (uint16_t)~(1u << it);
    else g_items2 &= (uint16_t)~(1u << (it - 16));
}
static uint8_t taken_has(uint8_t it) {
    if (it < 16) return (g_taken & (uint16_t)(1u << it)) != 0;
    return (g_taken2 & (uint16_t)(1u << (it - 16))) != 0;
}
static void taken_set(uint8_t it) {
    if (it < 16) g_taken |= (uint16_t)(1u << it);
    else g_taken2 |= (uint16_t)(1u << (it - 16));
}

/* --- audio: title music on channels 1+2+3, SFX on channel 1 in-game,
 * ticked once per frame. Frequencies are GB timer values:
 * f = 2048 - 131072 / Hz. Music plays ONLY on the title screen
 * (Designer: in-game music is annoying): the approved title theme,
 * an 8-bar A harmonic minor phrase in the demo's exact notes —
 * melody on pulse channel 2, harmony on pulse channel 1, bass on the
 * wave channel — quantized to the engine's frame timing (a beat is
 * 55 frames, ~65 BPM). It loops cleanly and stops dead when the game
 * starts. SFX are short table sequences on channel 1 and keep working
 * in-game. Headless QA cannot hear any of this; register activity is
 * asserted in qa/audio_regs.py instead. */
static const uint16_t title_mel[] = {
    /* C5 1.5, B4 .5, A4 2 | E4 1, G#4 1, A4 2 */
    1798, 82, 1783, 28, 1750, 110, 1650, 55, 1732, 55, 1750, 110,
    /* F4 2, A4 1, C5 1 | B4 1.5, G#4 .5, B4 2 */
    1673, 110, 1750, 55, 1798, 55, 1783, 82, 1732, 28, 1783, 110,
    /* C5 2, E5 1, D5 1 | C5 2, A4 1, F4 1 */
    1798, 110, 1849, 55, 1825, 55, 1798, 110, 1750, 55, 1673, 55,
    /* G#4 2, B4 2 | A4 4, rest 1 */
    1732, 110, 1783, 110, 1750, 220, 0, 55, 0, 0
};
static const uint16_t title_harm[] = {
    /* one chord per bar, third then fifth, 2 beats each */
    1798, 110, 1849, 110,   /* Am: C5 E5 */
    1798, 110, 1849, 110,   /* Am: C5 E5 */
    1750, 110, 1798, 110,   /* F:  A4 C5 */
    1732, 110, 1783, 110,   /* E:  G#4 B4 */
    1798, 110, 1849, 110,   /* Am: C5 E5 */
    1750, 110, 1798, 110,   /* F:  A4 C5 */
    1732, 110, 1783, 110,   /* E:  G#4 B4 */
    1798, 110, 1849, 110,   /* Am: C5 E5 */
    0, 55, 0, 0
};
static const uint16_t title_bass[] = {
    /* root twice per bar, 2 beats each */
    856, 110, 856, 110,     /* A2 */
    856, 110, 856, 110,     /* A2 */
    547, 110, 547, 110,     /* F2 */
    458, 110, 458, 110,     /* E2 */
    856, 110, 856, 110,     /* A2 */
    547, 110, 547, 110,     /* F2 */
    458, 110, 458, 110,     /* E2 */
    856, 110, 856, 110,     /* A2 */
    0, 55, 0, 0
};
/* Triangle wave table for the bass (32 4-bit samples, packed). */
static const uint8_t bass_wave[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
};
static const uint16_t music_seq[] = {
    0, 0 /* placeholder: the title tune lives in title_* above */
};
/* Sentinel marking "an ending track is playing" for music_tick;
   the actual ending tables are selected by do_ending. */
static const uint16_t end_seq[] = { 0, 0 };
/* Ending tracks, same three-channel treatment as the approved title
   theme: phrased melody (pulse 2, 50% duty, vol 7) + per-bar harmony
   (pulse 1, 25% duty, vol 5) + bass roots (wave channel, triangle
   table). A beat is 48 frames (~75 BPM); each theme is 8 bars and
   loops on its ending screen. PURE: C major, ascending, resolves
   home to C. TAINTED: A minor, ends held on the dominant E —
   unresolved. KEEPER: harmonic minor, melody sinking step by step
   to a long low A. Note values use the same GB timer formula as the
   title tune (C2 44, E2 458, F2 547, G2 711, A2 856, B2 986,
   C3 1046, D3 1155, E3 1253, F3 1297, G#3 1417, A3 1452, B3 1517,
   C4 1547, D4 1602, E4 1650, F4 1673, G4 1714, G#4 1732, A4 1750,
   B4 1783, C5 1798, D5 1825, E5 1849). */
static const uint16_t end_pure_mel[] = {
    1650, 72, 1714, 24, 1798, 96,
    1673, 96, 1750, 48, 1798, 48,
    1602, 48, 1714, 48, 1783, 96,
    1798, 96, 1849, 48, 1825, 48,
    1849, 72, 1825, 24, 1798, 96,
    1750, 96, 1673, 48, 1750, 48,
    1783, 96, 1825, 96,
    1798, 192, 0, 48, 0, 0
};
static const uint16_t end_pure_harm[] = {
    1650, 96, 1714, 96, 1750, 96, 1798, 96,
    1783, 96, 1825, 96, 1650, 96, 1714, 96,
    1650, 96, 1714, 96, 1750, 96, 1798, 96,
    1783, 96, 1825, 96, 1650, 96, 1714, 96,
    0, 48, 0, 0
};
static const uint16_t end_pure_bass[] = {
    44, 96, 44, 96, 547, 96, 547, 96,
    711, 96, 711, 96, 44, 96, 44, 96,
    44, 96, 44, 96, 547, 96, 547, 96,
    711, 96, 711, 96, 44, 96, 44, 96,
    0, 48, 0, 0
};
static const uint16_t end_tainted_mel[] = {
    1750, 96, 1798, 48, 1849, 48,
    1673, 72, 1750, 24, 1798, 96,
    1732, 96, 1783, 96,
    1750, 96, 1849, 48, 1825, 48,
    1798, 96, 1783, 48, 1750, 48,
    1798, 96, 1750, 96,
    1783, 96, 1732, 96,
    1849, 192, 0, 48, 0, 0
};
static const uint16_t end_tainted_harm[] = {
    1798, 96, 1849, 96, 1750, 96, 1798, 96,
    1732, 96, 1783, 96, 1798, 96, 1849, 96,
    1798, 96, 1849, 96, 1750, 96, 1798, 96,
    1732, 96, 1783, 96, 1732, 96, 1783, 96,
    0, 48, 0, 0
};
static const uint16_t end_tainted_bass[] = {
    856, 96, 856, 96, 547, 96, 547, 96,
    458, 96, 458, 96, 856, 96, 856, 96,
    856, 96, 856, 96, 547, 96, 547, 96,
    458, 96, 458, 96, 458, 96, 458, 96,
    0, 48, 0, 0
};
static const uint16_t end_keeper_mel[] = {
    1849, 96, 1825, 48, 1798, 48,
    1783, 96, 1750, 48, 1732, 48,
    1750, 96, 1673, 96,
    1650, 96, 1602, 96,
    1547, 96, 1517, 96,
    1452, 96, 1417, 96,
    1452, 96, 1297, 96,
    856, 192, 0, 48, 0, 0
};
static const uint16_t end_keeper_harm[] = {
    1798, 96, 1849, 96, 1732, 96, 1783, 96,
    1798, 96, 1849, 96, 1732, 96, 1783, 96,
    1798, 96, 1849, 96, 1732, 96, 1783, 96,
    1798, 96, 1849, 96, 1798, 96, 1849, 96,
    0, 48, 0, 0
};
static const uint16_t end_keeper_bass[] = {
    856, 96, 856, 96, 458, 96, 458, 96,
    856, 96, 856, 96, 458, 96, 458, 96,
    856, 96, 856, 96, 458, 96, 458, 96,
    856, 96, 856, 96, 856, 96, 856, 96,
    0, 48, 0, 0
};
static const uint16_t *mus_seq_cur = music_seq;
static uint8_t mus_idx, mus_timer, mus_on = 1;
/* Title-track player state (three parallel sequences). */
static uint8_t tit_mel_idx, tit_mel_timer;
static uint8_t tit_harm_idx, tit_harm_timer;
static uint8_t tit_bass_idx, tit_bass_timer;
/* Ending-track player state (same three-channel engine, tables
   selected by do_ending). Declared here, after all WRAM globals, so
   no existing WRAM address shifts. */
static const uint16_t *end_mel_cur, *end_harm_cur, *end_bass_cur;
static uint8_t end_mel_idx, end_mel_timer;
static uint8_t end_harm_idx, end_harm_timer;
static uint8_t end_bass_idx, end_bass_timer;
/* One title-track channel: chan 2 = melody (pulse, duty 50%, vol 7),
   chan 1 = harmony (pulse, duty 25%, vol 5), chan 3 = bass (wave). */
static void title_chan(const uint16_t *seq, uint8_t *idx, uint8_t *timer,
                       uint8_t chan) {
    uint16_t f;
    uint8_t l;
    if (*timer) { (*timer)--; return; }
    f = seq[*idx];
    l = (uint8_t)seq[*idx + 1];
    if (!l) { *idx = 0; return; }
    if (chan == 2) {
        if (!f) NR22_REG = 0x00;
        else {
            NR21_REG = 0x80;
            NR22_REG = 0x70;
            NR23_REG = (uint8_t)f;
            NR24_REG = 0x80 | (uint8_t)(f >> 8);
        }
    } else if (chan == 1) {
        if (!f) NR12_REG = 0x00;
        else {
            NR11_REG = 0x40;
            NR12_REG = 0x50;
            NR13_REG = (uint8_t)f;
            NR14_REG = 0x80 | (uint8_t)(f >> 8);
        }
    } else {
        if (!f) NR32_REG = 0x00;
        else {
            NR30_REG = 0x80;
            NR32_REG = 0x20;
            NR33_REG = (uint8_t)f;
            NR34_REG = 0x80 | (uint8_t)(f >> 8);
        }
    }
    *timer = l;
    *idx += 2;
}
static void music_tick(void) {
    if (!mus_on) return;
    /* Title screen: the three-channel theme. Ending screens: the
       selected ending's three-channel theme, same engine. */
    if (mus_seq_cur == music_seq) {
        title_chan(title_mel, &tit_mel_idx, &tit_mel_timer, 2);
        title_chan(title_harm, &tit_harm_idx, &tit_harm_timer, 1);
        title_chan(title_bass, &tit_bass_idx, &tit_bass_timer, 3);
        return;
    }
    title_chan(end_mel_cur, &end_mel_idx, &end_mel_timer, 2);
    title_chan(end_harm_cur, &end_harm_idx, &end_harm_timer, 1);
    title_chan(end_bass_cur, &end_bass_idx, &end_bass_timer, 3);
}

/* SFX sequences: pairs of (freq, frames), terminated by a zero length. */
static const uint16_t sfx_pickup[]    = {1849, 6, 1899, 12, 0, 0};
static const uint16_t sfx_unlock[]    = {1547, 6, 1650, 6, 1714, 14, 0, 0};
static const uint16_t sfx_sacrifice[] = {1452, 10, 857, 20, 0, 0};
/* Virtue-gate unlock sting: a short sinister harmonic-minor phrase
   for the moment a required gate opens on a sacrifice — a dark
   bargain being sealed. Phrased (short-short-long, rise, fall back
   to the tonic), ~2.2 seconds, played over the offer message so it
   never blocks gameplay. Optional pacts keep the old two-tone
   sfx_sacrifice; only required-gate unlocks get the sting. */
static const uint16_t sfx_gate_sting[] = {
    1750, 15, 1732, 10, 1750, 20, 1798, 15,
    1783, 15, 1732, 20, 1750, 30, 0, 0
};
static const uint16_t sfx_denied[]    = {300, 10, 120, 16, 0, 0};
/* Room-change wipe sound: a longer rising scale run in harmonic minor,
   ~22 frames to match the ~20-frame column wipe, Shadowgate-style.
   Distinct from the unlock arpeggio (3 notes) and the pickup (high). */
static const uint16_t sfx_wipe[]      = {
    710, 2, 856, 2, 986, 2, 1046, 2, 1155, 2, 1253, 2,
    1297, 2, 1417, 2, 1452, 2, 1547, 4, 0, 0
};
static const uint16_t *sfx_seq = 0;
static uint8_t sfx_idx, sfx_timer;
static void sfx_play(const uint16_t *seq) {
    sfx_seq = seq;
    sfx_idx = 0;
    sfx_timer = 0;
}
static void sfx_tick(void) {
    uint16_t f;
    uint8_t l;
    if (!sfx_seq) return;
    if (sfx_timer) { sfx_timer--; return; }
    f = sfx_seq[sfx_idx];
    l = (uint8_t)sfx_seq[sfx_idx + 1];
    if (!l) { sfx_seq = 0; NR12_REG = 0x00; return; }
    NR11_REG = 0x80;
    NR12_REG = 0xF0;
    NR13_REG = (uint8_t)f;
    NR14_REG = 0x80 | (uint8_t)(f >> 8);
    sfx_timer = l;
    sfx_idx += 2;
}

static const char * const virtue_names[8] = {
    "HUMILITY", "PATIENCE", "CHARITY", "TEMPERANCE",
    "CHASTITY", "DILIGENCE", "KINDNESS", "HOPE"
};
static const char * const sin_names[8] = {
    "PRIDE", "WRATH", "GREED", "GLUTTONY", "LUST", "SLOTH", "ENVY", "DESPAIR"
};
static const char * const item_short_names[26] = {
    "KEY", "ROPE", "CANDLE", "MIRROR", "BELL", "LADDER",
    "AXE", "PRISM", "OIL", "HORN", "OBSIDIAN", "SUNSTONE",
    "WOOD", "KEY BOW", "KEY STEM", "FINAL KEY",
    "STAR CHART", "LOFT KEY",
    "MOONSTONE", "TORCH", "ICE KEY", "PALE GEM", "BLADE", "KEEPER'S SEAL",
    "STAR KEY", "DROWNED KEY"
};
/* 32KB-fit trims (backtrack-lock build): the Star/Drowned Key take
   messages are bare ("TAKEN: STAR KEY." / "TAKEN: DROWNED KEY."),
   and the Moonstone, Pale Gem, Blade, Torch, Rope, Chart, Loft Key
   and Horn take messages lost their destination hints — those hints
   survive in the take-denial messages, the use/door denials, and
   the shield/tablet looks, so no puzzle signposting is lost.
   MBC5 was tried and reverted: GBDK non-banked code must still fit
   the 32KB window (code + the trailing HOME segment). */
static const char * const take_msgs[26] = {
    "TAKEN: RUSTY KEY.", "TAKEN: COILED ROPE.", "TAKEN: CANDLE.",
    "TAKEN: HAND MIRROR.", "TAKEN: SILVER BELL.", "TAKEN: LADDER.",
    "TAKEN: AXE.", "TAKEN: PRISM.", "TAKEN: OIL FLASK.",
    "TAKEN: CHOIR HORN.",
    "TAKEN: OBSIDIAN KEY.", "TAKEN: SUNSTONE.",
    "TAKEN: DEAD WOOD.", "TAKEN: KEY BOW.", "TAKEN: KEY STEM.",
    "TAKEN: FINAL KEY.", "TAKEN: STAR CHART.",
    "TAKEN: LOFT KEY.",
    "TAKEN: MOONSTONE. IT GLOWS WITHOUT FIRE.",
    "TAKEN: EMBER TORCH.",
    "TAKEN: ICE KEY.", "TAKEN: PALE GEM.",
    "TAKEN: SILVER BLADE.",
    "TAKEN: KEEPER'S SEAL. THE FINAL GATE WILL KNOW YOU.",
    "TAKEN: STAR KEY.",
    "TAKEN: DROWNED KEY."
};
static const char * const pact_msgs[5] = {
    "THE PORTRAIT WHISPERS: 'ONLY A PURE TONE STIRS THE GUARDIAN.'",
    "THE SKULL GRINS: 'BLACK GLASS YIELDS ONLY TO ITS OWN FACE.'",
    "THE ALTAR DRINKS DEEP. SOMETHING IN THE DARK LEARNS YOUR NAME.",
    "THE SHRINE SHOWS YOU YOUR OWN FACE, SMILING WITHOUT YOU.",
    "YOU KNEEL. THE THRONE REMEMBERS THE WEIGHT OF YOU."
};
static const uint8_t gate_target[5] = {5, 7, 42, 9, 255};
static const uint8_t gate_req[5] = {
    RQ_NONE, RQ_NONE, RQ_NONE, RQ_BELL, RQ_NONE
};

typedef struct {
    uint8_t x, y, type, param, lock;
    const char *name;
    const char *look;
} Hotspot;

static const Hotspot hs_gatehouse[] = {
    {5, 2, HS_TAKE, IT_KEY, 0, "RUSTY KEY", ""},
    {14, 2, HS_DOOR, 1, LK_KEY, "CASTLE GATE", ""},
    {13, 6, HS_LOOK, 0, 0, "PLAQUE",
     "THE PLAQUE READS: 'ALL WHO ENTER PAY. ALL WHO LEAVE PAY MORE.'"},
};
static const Hotspot hs_courtyard[] = {
    {3, 2, HS_USE, USE_WELL, 0, "THE WELL", ""},
    {16, 3, HS_USE, USE_TREE, 0, "DEAD TREE", ""},
    {16, 2, HS_DOOR, 2, 0, "COURTYARD GATE", ""},
    {10, 10, HS_DOOR, 0, 0, "GATEHOUSE DOOR", ""},
    {3, 8, HS_DOOR, 19, 0, "PATIO DOOR", ""},
};
static const Hotspot hs_hall[] = {
    {3, 7, HS_DOOR, 3, 0, "WEST DOOR", ""},
    {15, 7, HS_DOOR, 4, 0, "EAST DOOR", ""},
    {10, 2, HS_GATE, 0, 0, "SEALED GATE", ""},
    {10, 10, HS_DOOR, 1, 0, "COURTYARD DOOR", ""},
};
static const Hotspot hs_kitchen[] = {
    {8, 2, HS_TAKE, IT_MIRROR, 0, "HAND MIRROR", ""},
    {3, 4, HS_LOOK, 0, 0, "KITCHEN TABLE",
     "DUST AND OLD BREAD. NOTHING USEFUL."},
    {16, 7, HS_DOOR, 2, 0, "HALL DOOR", ""},
    {15, 2, HS_DOOR, 17, 0, "PANTRY DOOR", ""},
};
static const Hotspot hs_library[] = {
    {16, 2, HS_PACT, 0, 0, "PORTRAIT", ""},
    {3, 4, HS_TAKE, IT_PRISM, LK_CHART, "PRISM", ""},
    {3, 7, HS_DOOR, 2, 0, "HALL DOOR", ""},
};
static const Hotspot hs_chapel[] = {
    {9, 2, HS_PACT, 2, 0, "ALTAR", ""},
    {3, 4, HS_LOOK, 0, 0, "FONT",
     "THE FONT IS DRY. ONCE IT HELD HOLY WATER."},
    {16, 4, HS_DOOR, 6, LK_LADDER, "STAIR DOWN", ""},
    {9, 8, HS_GATE, 1, 0, "SEALED GATE", ""},
    {9, 10, HS_DOOR, 2, 0, "HALL DOOR", ""},
};
static const Hotspot hs_crypt[] = {
    {9, 7, HS_TAKE, IT_BELL, 0, "SILVER BELL", ""},
    {9, 2, HS_PACT, 1, 0, "WHISPERING SKULL", ""},
    {16, 4, HS_DOOR, 5, 0, "STAIR UP", ""},
    {3, 4, HS_LOOK, 0, 0, "OLD BONES",
     "THE BONES OF A PILGRIM WHO PAID TOO MUCH."},
    {16, 7, HS_USE, USE_COFFER, 0, "SEALED COFFER", ""},
};
static const Hotspot hs_gallery[] = {
    {9, 2, HS_USE, USE_MIRROR, 0, "WALL OF GLASS", ""},
    {9, 8, HS_DOOR, 8, LK_MIRROR, "SEALED GATE", ""},
    {3, 6, HS_PACT, 3, 0, "SHRINE", ""},
    {16, 2, HS_TAKE, IT_LADDER, 0, "LADDER", ""},
    {9, 10, HS_DOOR, 5, 0, "CHAPEL DOOR", ""},
    {3, 2, HS_DOOR, 18, 0, "ARMORY DOOR", ""},
};
static const Hotspot hs_stair[] = {
    {9, 2, HS_USE, USE_BELL, 0, "STONE GUARDIAN", ""},
    {9, 8, HS_DOOR, 10, LK_BELL, "SEALED GATE", ""},
    {9, 10, HS_DOOR, 7, 0, "GALLERY DOOR", ""},
};
static const Hotspot hs_vestibule[] = {
    {9, 2, HS_EXIT, 0, 0, "THE FINAL GATE", ""},
    {3, 6, HS_PACT, 4, 0, "THE KEEPER'S THRONE", ""},
    {3, 10, HS_DOOR, 8, 0, "STAIR DOOR", ""},
};
static const Hotspot hs_observatory[] = {
    {9, 2, HS_USE, USE_LENS, 0, "STAR LENS", ""},
    {3, 4, HS_LOOK, 0, 0, "STAR WINDOW",
     "THROUGH THE WINDOW, STARS SPELL A WARNING: WHAT YOU GIVE, YOU BECOME."},
    {9, 8, HS_DOOR, 11, LK_LENS, "LENS GATE", ""},
    {9, 10, HS_DOOR, 8, 0, "STAIR DOOR", ""},
    {3, 8, HS_DOOR, 22, 0, "CHART ROOM DOOR", ""},
    {16, 2, HS_TAKE, IT_MOONSTONE, LK_STARKEY, "MOONSTONE", ""},
};
static const Hotspot hs_chasm[] = {
    {10, 5, HS_USE, USE_WOOD, 0, "THE CHASM GAP", ""},
    {9, 8, HS_DOOR, 12, LK_BRIDGE, "CHASM CROSSING", ""},
    {3, 4, HS_LOOK, 0, 0, "THE CHASM",
     "THE CHASM IS WIDE AND DARK. WOOD COULD BRIDGE IT."},
    {9, 10, HS_DOOR, 10, 0, "OBSERVATORY DOOR", ""},
};
static const Hotspot hs_archive[] = {
    {9, 8, HS_DOOR, 13, LK_OIL, "RUSTED DOOR", ""},
    {3, 4, HS_LOOK, 0, 0, "DUSTY TOMES",
     "THE TOMES WARN: THE KEEPER WAS ONCE A PILGRIM, LIKE YOU."},
    {9, 10, HS_DOOR, 11, 0, "CHASM DOOR", ""},
    {3, 7, HS_DOOR, 15, 0, "SMITHY DOOR", ""},
};
static const Hotspot hs_echo[] = {
    {9, 2, HS_USE, USE_ECHO, 0, "SLEEPING GATEKEEPER", ""},
    {3, 4, HS_LOOK, 0, 0, "PILLAR",
     "THE PILLARS THROW YOUR FOOTSTEPS BACK AT YOU."},
    {9, 8, HS_DOOR, 14, LK_ECHO, "ECHO GATE", ""},
    {9, 10, HS_DOOR, 12, 0, "ARCHIVE DOOR", ""},
    {3, 2, HS_DOOR, 16, 0, "BELFRY DOOR", ""},
    {16, 8, HS_DOOR, 23, LK_HORNKEY, "CHOIR LOFT DOOR", ""},
    {16, 10, HS_DOOR, 24, 0, "PASSAGE DOOR", ""},
};
static const Hotspot hs_reliquary[] = {
    {9, 8, HS_DOOR, 9, LK_OBSIDIAN, "SANCTUM DOOR", ""},
    {9, 2, HS_LOOK, 0, 0, "THE RELIC",
     "A RELIC OF THE FIRST KEEPER. IT IS WARM, LIKE A HAND."},
    {3, 4, HS_LOOK, 0, 0, "VOTIVE FLAME",
     "THE FLAME DOES NOT FLICKER. IT WATCHES."},
    {9, 10, HS_DOOR, 13, 0, "ECHO DOOR", ""},
    {16, 7, HS_DOOR, 31, 0, "DROWNED WING DOOR", ""},
};
static const Hotspot hs_smithy[] = {
    {3, 2, HS_TAKE, IT_BOW, 0, "KEY BOW", ""},
    {9, 2, HS_USE, USE_FORGE, 0, "THE FORGE", ""},
    {16, 7, HS_DOOR, 12, 0, "ARCHIVE DOOR", ""},
};
static const Hotspot hs_belfry[] = {
    {3, 2, HS_TAKE, IT_STEM, 0, "KEY STEM", ""},
    {16, 7, HS_DOOR, 13, 0, "ECHO DOOR", ""},
};
static const Hotspot hs_pantry[] = {
    {3, 4, HS_LOOK, 0, 0, "EMPTY SHELVES",
     "THE SHELVES ARE BARE. A LARDER DOOR LEADS DEEPER."},
    {16, 7, HS_DOOR, 3, 0, "KITCHEN DOOR", ""},
    {9, 8, HS_DOOR, 29, 0, "LARDER DOOR", ""},
    {3, 2, HS_TAKE, IT_BLADE, LK_DROWNEDKEY, "SILVER BLADE", ""},
};
static const Hotspot hs_armory[] = {
    {3, 2, HS_TAKE, IT_AXE, 0, "AXE", ""},
    {16, 7, HS_DOOR, 7, 0, "GALLERY DOOR", ""},
};
static const Hotspot hs_patio[] = {
    {3, 2, HS_LOOK, 0, 0, "CRACKED STATUE",
     "THE STATUE'S FACE IS GONE. SOMETHING TOOK IT."},
    {16, 4, HS_LOOK, 0, 0, "DEAD ROSES",
     "THE ROSES DIED STANDING UP. NOBODY CUT THEM DOWN."},
    {9, 10, HS_DOOR, 1, 0, "COURTYARD DOOR", ""},
    {9, 8, HS_DOOR, 20, 0, "GARDEN DOOR", ""},
};
static const Hotspot hs_garden[] = {
    {3, 2, HS_LOOK, 0, 0, "STONE ANGEL",
     "THE ANGEL WEEPS. ITS HANDS ARE EMPTY."},
    {16, 4, HS_LOOK, 0, 0, "OVERGROWN HEDGE",
     "THE HEDGE HAS GROWN OVER THE PATH. SOMETHING WATCHES FROM IT."},
    {9, 10, HS_DOOR, 19, 0, "PATIO DOOR", ""},
    {9, 8, HS_DOOR, 21, 0, "SHED DOOR", ""},
};
static const Hotspot hs_shed[] = {
    {3, 2, HS_TAKE, IT_ROPE, 0, "COILED ROPE", ""},
    {16, 4, HS_LOOK, 0, 0, "RUSTED TOOLS",
     "RUSTED TOOLS. SOMETHING COILED HANGS IN THE BACK."},
    {9, 10, HS_DOOR, 20, 0, "GARDEN DOOR", ""},
};
static const Hotspot hs_chartroom[] = {
    {3, 2, HS_TAKE, IT_CHART, 0, "STAR CHART", ""},
    {16, 4, HS_LOOK, 0, 0, "CELESTIAL GLOBE",
     "THE GLOBE SHOWS STARS THAT ARE NOT IN OUR SKY."},
    {9, 10, HS_DOOR, 10, 0, "OBSERVATORY DOOR", ""},
};
static const Hotspot hs_choir[] = {
    {3, 2, HS_TAKE, IT_HORN, 0, "CHOIR HORN", ""},
    {16, 4, HS_LOOK, 0, 0, "SILENT ORGAN",
     "THE ORGAN'S PIPES ARE STUFFED WITH CLOTH. NO MUSIC HERE."},
    {9, 10, HS_DOOR, 13, 0, "ECHO DOOR", ""},
};
static const Hotspot hs_pass1[] = {
    {3, 4, HS_LOOK, 0, 0, "WEEPING WALLS",
     "THE WALLS WEEP. THE PASSAGE WINDS ON."},
    {9, 10, HS_DOOR, 13, 0, "ECHO DOOR", ""},
    {9, 8, HS_DOOR, 25, 0, "DEEPER PASSAGE", ""},
};
static const Hotspot hs_pass2[] = {
    {3, 4, HS_LOOK, 0, 0, "TALLY MARKS",
     "SOMEONE CARVED TALLY MARKS. THEY STOP SUDDENLY."},
    {9, 10, HS_DOOR, 24, 0, "BACK PASSAGE", ""},
    {9, 8, HS_DOOR, 26, 0, "DEEPER PASSAGE", ""},
};
static const Hotspot hs_pass3[] = {
    {3, 4, HS_LOOK, 0, 0, "THICK DARK",
     "THE DARK HERE IS THICKER. IT LEANS ON YOU."},
    {9, 10, HS_DOOR, 25, 0, "BACK PASSAGE", ""},
    {9, 8, HS_DOOR, 27, 0, "DEEPER PASSAGE", ""},
};
static const Hotspot hs_pass4[] = {
    {3, 4, HS_LOOK, 0, 0, "OLD BONES",
     "THE BONES OF SOMEONE WHO NEVER CAME BACK."},
    {9, 10, HS_DOOR, 26, 0, "BACK PASSAGE", ""},
    {9, 8, HS_DOOR, 28, 0, "DEEP CELL DOOR", ""},
};
static const Hotspot hs_deepcell[] = {
    {3, 2, HS_TAKE, IT_LOFTKEY, 0, "LOFT KEY", ""},
    {16, 4, HS_LOOK, 0, 0, "EMPTY CHAINS",
     "CHAINS, EMPTY. WHATEVER THEY HELD GOT OUT."},
    {9, 10, HS_DOOR, 27, 0, "PASSAGE DOOR", ""},
};
static const Hotspot hs_larder[] = {
    {3, 2, HS_LOOK, 0, 0, "DEAD HERBS",
     "DEAD HERBS. A CELLAR DOOR LEADS DOWN."},
    {9, 10, HS_DOOR, 17, 0, "PANTRY DOOR", ""},
    {9, 8, HS_DOOR, 30, 0, "CELLAR DOOR", ""},
};
static const Hotspot hs_cellar[] = {
    {3, 2, HS_TAKE, IT_OIL, 0, "OIL FLASK", ""},
    {16, 4, HS_LOOK, 0, 0, "WINE CASKS",
     "WINE CASKS, LONG SOURED. SOMETHING ELSE LIVES DOWN HERE NOW."},
    {9, 10, HS_DOOR, 29, 0, "LARDER DOOR", ""},
};

static const Hotspot hs_wing_ante[] = {
    {3, 4, HS_LOOK, 0, 0, "DROWNED BANNER",
     "THE BANNER IS SOAKED THROUGH. THE OBSERVATORY HOLDS A RIDDLE'S ANSWER."},
    {9, 8, HS_DOOR, 32, 0, "FLOODED GALLERY DOOR", ""},
    {9, 10, HS_DOOR, 14, 0, "RELIQUARY DOOR", ""},
};
static const Hotspot hs_wing_gallery[] = {
    {3, 4, HS_LOOK, 0, 0, "FLOODED TAPESTRY",
     "THE TAPESTRY HANGS HEAVY WITH WATER."},
    {9, 8, HS_DOOR, 33, 0, "SUNKEN CHAPEL DOOR", ""},
    {9, 10, HS_DOOR, 31, 0, "ANTECHAMBER DOOR", ""},
    {16, 7, HS_DOOR, 36, 0, "TORCH VAULT DOOR", ""},
};
static const Hotspot hs_wing_chapel[] = {
    {3, 4, HS_LOOK, 0, 0, "SUNKEN ALTAR",
     "THE ALTAR STANDS IN WATER."},
    {9, 8, HS_DOOR, 34, 0, "WELL CHAMBER DOOR", ""},
    {9, 10, HS_DOOR, 32, 0, "GALLERY DOOR", ""},
    {3, 2, HS_TAKE, IT_STARKEY, 0, "STAR KEY", ""},
};
static const Hotspot hs_wing_well[] = {
    {9, 8, HS_DOOR, 35, 0, "ICE CHAMBER DOOR", ""},
    {9, 10, HS_DOOR, 33, 0, "CHAPEL DOOR", ""},
};
static const Hotspot hs_wing_ice[] = {
    {9, 2, HS_USE, USE_POOL, 0, "FROZEN POOL", ""},
    {9, 8, HS_DOOR, 38, LK_ICEKEY, "ICE DOOR", ""},
    {9, 10, HS_DOOR, 34, 0, "WELL CHAMBER DOOR", ""},
};
static const Hotspot hs_wing_torch[] = {
    {3, 2, HS_TAKE, IT_TORCH, 0, "EMBER TORCH", ""},
    {9, 10, HS_DOOR, 32, 0, "GALLERY DOOR", ""},
};
static const Hotspot hs_wing_gem[] = {
    {3, 2, HS_TAKE, IT_GEM, 0, "PALE GEM", ""},
    {9, 10, HS_DOOR, 38, 0, "SLOT HALL DOOR", ""},
};
static const Hotspot hs_wing_slot[] = {
    {9, 2, HS_USE, USE_GEMSLOT, 0, "GEM SLOT", ""},
    {9, 8, HS_DOOR, 39, LK_GEM, "HIDDEN DOOR", ""},
    {9, 10, HS_DOOR, 35, 0, "ICE CHAMBER DOOR", ""},
    {16, 7, HS_DOOR, 37, 0, "GEM GALLERY DOOR", ""},
    {3, 2, HS_TAKE, IT_DROWNEDKEY, 0, "DROWNED KEY", ""},
};
static const Hotspot hs_wing_riddle[] = {
    {9, 2, HS_USE, USE_RIDDLE, 0, "RIDDLE GUARDIAN", ""},
    {3, 4, HS_LOOK, 0, 0, "RIDDLE TABLET",
     "THE TABLET READS: 'WHAT GLOWS WITHOUT FIRE?' THE OBSERVATORY HOLDS THE ANSWER."},
    {9, 8, HS_DOOR, 40, LK_RIDDLE, "RIDDLE DOOR", ""},
    {9, 10, HS_DOOR, 38, 0, "SLOT HALL DOOR", ""},
};
static const Hotspot hs_wing_knight[] = {
    {9, 2, HS_USE, USE_KNIGHT, 0, "DROWNED KNIGHT", ""},
    {3, 4, HS_LOOK, 0, 0, "KNIGHT'S SHIELD",
     "THE KNIGHT DOES NOT MOVE. A BLADE WAITS IN THE PANTRY, "
     "LOCKED."},
    {9, 8, HS_DOOR, 41, LK_KNIGHT, "KNIGHT'S DOOR", ""},
    {9, 10, HS_DOOR, 39, 0, "RIDDLE HALL DOOR", ""},
};
static const Hotspot hs_wing_hope[] = {
    {9, 2, HS_GATE, 2, 0, "SEAL OF HOPE", ""},
    {3, 4, HS_LOOK, 0, 0, "HOPE'S FLAME",
     "A FLAME THAT NEVER WENT OUT. THE SEAL DEMANDS HOPE, AND DESPAIR TAKES ITS PLACE."},
    {9, 10, HS_DOOR, 40, 0, "KNIGHT'S GUARD DOOR", ""},
};
static const Hotspot hs_wing_ante2[] = {
    {3, 4, HS_LOOK, 0, 0, "KEEPER'S DESK",
     "THE DESK WARNS: 'THE LAST DOOR OPENS ONLY FOR ONE WHO BEARS THE SEAL.'"},
    {9, 8, HS_DOOR, 43, 0, "SEAL CHAMBER DOOR", ""},
    {9, 10, HS_DOOR, 41, 0, "HOPE CHAPEL DOOR", ""},
};
static const Hotspot hs_wing_seal[] = {
    {3, 2, HS_TAKE, IT_SEAL, 0, "KEEPER'S SEAL", ""},
    {9, 10, HS_DOOR, 42, 0, "ANTECHAMBER DOOR", ""},
};

static const Hotspot * const hs_rooms[44] = {
    hs_gatehouse, hs_courtyard, hs_hall, hs_kitchen, hs_library,
    hs_chapel, hs_crypt, hs_gallery, hs_stair, hs_vestibule,
    hs_observatory, hs_chasm, hs_archive, hs_echo, hs_reliquary,
    hs_smithy, hs_belfry, hs_pantry, hs_armory,
    hs_patio, hs_garden, hs_shed, hs_chartroom, hs_choir,
    hs_pass1, hs_pass2, hs_pass3, hs_pass4, hs_deepcell,
    hs_larder, hs_cellar,
    hs_wing_ante, hs_wing_gallery, hs_wing_chapel, hs_wing_well,
    hs_wing_ice, hs_wing_torch, hs_wing_gem, hs_wing_slot,
    hs_wing_riddle, hs_wing_knight, hs_wing_hope, hs_wing_ante2, hs_wing_seal
};
static const uint8_t hs_counts[44] = {3, 5, 4, 4, 3, 5, 5, 6, 3, 3, 6, 4, 4, 7, 5, 3, 2, 4, 2, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 2, 3, 2, 2, 5, 4, 4, 3, 3, 2};

/* message paging */
#define MSG_MAX_LINES 40
static char msg_lines[MSG_MAX_LINES][21];
static uint8_t msg_nlines, msg_page;
static char msg_buf[120];
static uint8_t sac_list[8], sac_count;

static uint8_t glyph_idx(char c) {
    if (c >= 'A' && c <= 'Z') return (uint8_t)(c - 'A');
    if (c >= 'a' && c <= 'z') return (uint8_t)(c - 'a');
    if (c >= '0' && c <= '9') return (uint8_t)(26 + c - '0');
    switch (c) {
        case ' ': return 36;
        case '.': return 37;
        case ',': return 38;
        case '!': return 39;
        case '?': return 40;
        case '\'': return 41;
        case '-': return 42;
        case ':': return 43;
        case ';': return 44;
        case '(': return 45;
        case ')': return 46;
        case '/': return 47;
        case '&': return 48;
        case '>': return 49;
        case '<': return 50;
        case '+': return 51;
        case '=': return 52;
        case '"': return 53;
    }
    return 36;
}

static void draw_str(uint8_t x, uint8_t y, const char *s) {
    while (*s && x < 20) {
        set_bkg_tile_xy(x, y, FONT_BASE + glyph_idx(*s));
        x++;
        s++;
    }
}

static void clear_screen(void) {
    uint8_t x, y;
    for (y = 0; y < 18; y++)
        for (x = 0; x < 20; x++)
            set_bkg_tile_xy(x, y, T_BLANK);
}

static void wrap_message(const char *msg) {
    uint8_t li = 0, ci = 0, wi, k;
    while (*msg) {
        while (*msg == ' ') msg++;
        if (!*msg) break;
        wi = 0;
        while (msg[wi] && msg[wi] != ' ') wi++;
        if (ci > 0 && ci + 1 + wi > 20) {
            msg_lines[li][ci] = 0;
            li++;
            ci = 0;
            if (li >= MSG_MAX_LINES) { li = MSG_MAX_LINES - 1; break; }
        }
        if (ci > 0) msg_lines[li][ci++] = ' ';
        for (k = 0; k < wi; k++) {
            if (ci >= 20) {
                msg_lines[li][ci] = 0;
                li++;
                ci = 0;
                if (li >= MSG_MAX_LINES) break;
            }
            msg_lines[li][ci++] = msg[k];
        }
        msg += wi;
    }
    msg_lines[li][ci] = 0;
    msg_nlines = li + 1;
    msg_page = 0;
}

static void draw_msg_page(void) {
    uint8_t r, li, x;
    for (r = 0; r < 5; r++) {
        li = msg_page * 5 + r;
        for (x = 0; x < 20; x++) set_bkg_tile_xy(x, 13 + r, T_BLANK);
        if (li < msg_nlines) draw_str(0, 13 + r, msg_lines[li]);
    }
}

static void show_message(const char *msg) {
    wrap_message(msg);
    draw_msg_page();
    g_state = ST_TEXT;
}

static uint8_t char_tile(char c, uint8_t twisted) {
    switch (c) {
        case '#': return twisted ? T_WALL_DARK : T_WALL;
        case '.': return twisted ? T_FLOOR_DARK : T_FLOOR;
        case 'D': return twisted ? T_DOOR_DARK : T_DOOR;
        case 'W': return twisted ? T_WINDOW_EYE : T_WINDOW;
        case 'A': return T_ALTAR;
        case 'T': return T_TABLE;
        case 'K': return T_KEY;
        case 'C': return T_CANDLE;
        case 'M': return T_MIRROR;
        case 'B': return T_BELL;
        case 'R': return T_ROPE;
        case 'O': return T_WELL;
        case 'S': return T_SHELF;
        case 'X': return T_SKULL;
        case 'F': return T_FONT;
        case 'E': return T_STAIRS;
        case 'G': return T_GATE;
        case 'P': return T_PORTRAIT;
        case 'H': return T_THRONE;
        case 'Y': return T_EYE;
        case '!': return T_CRACK;
        case 'L': return T_FLAME;
        case 'Z': return T_EXITDOOR;
        case '|': return T_PILLAR;
        case '$': return T_CHEST;
        case 'U': return T_BONES;
        case '~': return T_WEB;
        case '^': return T_STATUE;
        case '_': return T_RUG;
        case '%': return T_DARK;
        case 'l': return T_LADDER;
        case 'a': return T_AXE;
        case 'n': return T_PRISM;
        case 'j': return T_OIL;
        case 'q': return T_HORN;
        case 'k': return T_OBSIDIAN;
        case 'v': return T_SUNSTONE;
        case 't': return T_TREE;
        case 'b': return T_BOW;
        case 's': return T_STEM;
        case 'h': return T_CHART;
        case 'f': return T_LOFTKEY;
        case 'c': return T_CHAIN;
        default: return T_BLANK;
    }
}

static void draw_hot_hint(void) {
    uint8_t x;
    const Hotspot *h;
    for (x = 0; x < 20; x++) {
        set_bkg_tile_xy(x, 13, T_BLANK);
        set_bkg_tile_xy(x, 14, T_BLANK);
        set_bkg_tile_xy(x, 15, T_BLANK);
        set_bkg_tile_xy(x, 16, T_BLANK);
        set_bkg_tile_xy(x, 17, T_BLANK);
    }
    h = &hs_rooms[g_room][g_hot];
    draw_str(0, 13, h->name);
    if (h->type == HS_TAKE) draw_str(0, 14, "A: TAKE");
    else if (h->type == HS_DOOR) draw_str(0, 14, "A: GO");
    else if (h->type == HS_GATE)
        draw_str(0, 14, (g_gates & (1 << h->param)) ? "A: GO" : "A: OFFER");
    else if (h->type == HS_PACT) draw_str(0, 14, "A: LISTEN");
    else if (h->type == HS_USE) draw_str(0, 14, "A: USE");
    else if (h->type == HS_EXIT) draw_str(0, 14, "A: UNLOCK");
    else draw_str(0, 14, "A: LOOK");
    draw_str(0, 16, "START: ITEMS");
}

static void draw_room(void) {
    uint8_t x, y, twisted = (g_sins >= 4), i, cnt;
    const Hotspot *hh;
    for (y = 0; y < 12; y++)
        for (x = 0; x < 20; x++)
            set_bkg_tile_xy(x, y, char_tile(room_maps[g_room][y][x], twisted));
    /* Round-4 fix: a taken item's sprite must vanish from the room
       map, keyed off g_taken (never cleared) — NOT g_items, whose
       bits clear on consumption and used to make used items respawn.
       Round-5: taken_has covers the second bitmask word too. */
    cnt = hs_counts[g_room];
    for (i = 0; i < cnt; i++) {
        hh = &hs_rooms[g_room][i];
        if (hh->type == HS_TAKE && taken_has(hh->param))
            set_bkg_tile_xy(hh->x, hh->y,
                twisted ? T_FLOOR_DARK : T_FLOOR);
    }
    /* Round-5: once the chasm bridge is built, a log lies across the
       gap row (row 5) so the crossing is visible, not just text. */
    if (g_room == 11 && g_bridge_built)
        for (x = 1; x < 19; x++)
            set_bkg_tile_xy(x, 5, T_BRIDGE);
    for (x = 0; x < 20; x++) set_bkg_tile_xy(x, 12, T_DIV);
    draw_hot_hint();
}

static uint8_t hs_active(uint8_t r, uint8_t i) {
    const Hotspot *h = &hs_rooms[r][i];
    /* Round-4 fix: takeability keys off g_taken, never off g_items,
       so a consumed item can never become takeable again. Round-5:
       taken_has covers the second bitmask word too. */
    if (h->type == HS_TAKE) return !taken_has(h->param);
    if (h->type == HS_USE && h->param == USE_WELL)
        return !g_well_used;
    return 1;
}

/* Cursor placement: GBDK move_sprite takes screen coords MINUS 8 (x)
   and MINUS 16 (y) — the raw tile pixel coords used before put the
   diamond a full tile left and two tiles up from the hotspot, which is
   why it looked like it floated at a distance. Place it immediately
   LEFT of the hotspot tile, same row, edge touching edge, so it reads
   as pointing at the item. (No hotspot sits at x == 0; the guard
   covers that case by pointing from the right instead.) */
static void place_cursor(void) {
    const Hotspot *h = &hs_rooms[g_room][g_hot];
    if (h->x == 0) move_sprite(0, h->x * 8 + 16, h->y * 8 + 16);
    else move_sprite(0, h->x * 8, h->y * 8 + 16);
}

static void hs_move(int8_t dir) {
    uint8_t cnt = hs_counts[g_room];
    uint8_t i = g_hot, k;
    for (k = 0; k < cnt; k++) {
        if (dir > 0) { i++; if (i >= cnt) i = 0; }
        else { if (i == 0) i = cnt - 1; else i--; }
        if (hs_active(g_room, i)) {
            g_hot = i;
            draw_hot_hint();
            place_cursor();
            return;
        }
    }
}

static void draw_room_col(uint8_t x) {
    uint8_t y, twisted = (g_sins >= 4), i, cnt;
    const Hotspot *hh;
    for (y = 0; y < 12; y++)
        set_bkg_tile_xy(x, y, char_tile(room_maps[g_room][y][x], twisted));
    /* Same taken-item blanking as draw_room, for this column only. */
    cnt = hs_counts[g_room];
    for (i = 0; i < cnt; i++) {
        hh = &hs_rooms[g_room][i];
        if (hh->type == HS_TAKE && hh->x == x && taken_has(hh->param))
            set_bkg_tile_xy(x, hh->y,
                twisted ? T_FLOOR_DARK : T_FLOOR);
    }
    /* Same bridge override as draw_room, for this column only: the
       wipe reveals the log along with the rest of the room. */
    if (g_room == 11 && g_bridge_built && x >= 1 && x < 19)
        set_bkg_tile_xy(x, 5, T_BRIDGE);
    set_bkg_tile_xy(x, 12, T_DIV);
    for (y = 13; y < 18; y++)
        set_bkg_tile_xy(x, y, T_BLANK);
}

static void enter_room(uint8_t r) {
    uint8_t x;
    g_room = r;
    g_hot = 0;
    if (!hs_active(r, 0)) hs_move(1);
    /* Screen wipe: reveal the new room column by column, left to right.
       One column per vblank (~20 frames, ~0.33s) so it feels snappy.
       Each column is written during vblank, so no tearing; the joypad is
       not polled mid-wipe, so no input is accepted until the new room is
       fully revealed; music/SFX ticks keep running, so audio is
       uninterrupted. The final frame is identical to draw_room().
       The wipe sound (a rising harmonic-minor scale run) plays across
       the wipe itself. */
    move_sprite(0, 200, 200);
    sfx_play(sfx_wipe);
    for (x = 0; x < 20; x++) {
        wait_vbl_done();
        music_tick();
        sfx_tick();
        draw_room_col(x);
    }
    draw_hot_hint();
}

static uint8_t gates_remaining(void) {
    /* Gates 0, 1 and 2 demand a sacrifice; the rest open via items. */
    uint8_t n = 0, g;
    for (g = 0; g < 3; g++) if (!(g_gates & (1 << g))) n++;
    return n;
}

static uint8_t virtues_remaining(void) {
    uint8_t n = 0, v;
    for (v = 0; v < 8; v++) if (g_virtues & (1 << v)) n++;
    return n;
}

static void build_sac_list(void) {
    uint8_t v;
    sac_count = 0;
    /* The Hope seal (gate 2) offers Hope alone, so that gate still
       demands Hope specifically. Every other sacrifice offers all
       remaining virtues, including Hope: if Hope was already spent
       early, the Hope gate opens on its own (see the gid == 2 guard
       in the HS_GATE handler), so spending Hope early can never
       softlock the game. */
    if (!g_pending_is_pact && g_pending_id == 2) {
        if (g_virtues & (1 << 7)) sac_list[sac_count++] = 7;
        return;
    }
    for (v = 0; v < 8; v++)
        if (g_virtues & (1 << v)) sac_list[sac_count++] = v;
}

static void draw_sac_menu(void) {
    uint8_t i, row = 3, v, ci;
    char line[21];
    clear_screen();
    draw_str(3, 1, "OFFER A VIRTUE");
    for (i = 0; i < sac_count; i++) {
        v = sac_list[i];
        draw_str(0, row, (i == g_sac_sel) ? ">" : " ");
        ci = 0;
        { const char *s = virtue_names[v]; while (*s) line[ci++] = *s++; }
        line[ci++] = '>';
        { const char *s = sin_names[v]; while (*s) line[ci++] = *s++; }
        line[ci] = 0;
        draw_str(1, row, line);
        row++;
    }
    draw_str(0, 15, "B: KEEP WHAT REMAINS");
    draw_str(0, 16, "A: OFFER D-PAD: PICK");
}

static void build_offer_msg(uint8_t v) {
    uint8_t i = 0;
    const char *s;
    s = "YOU OFFER ";
    while (*s) msg_buf[i++] = *s++;
    s = virtue_names[v];
    while (*s) msg_buf[i++] = *s++;
    s = ". ";
    while (*s) msg_buf[i++] = *s++;
    s = sin_names[v];
    while (*s) msg_buf[i++] = *s++;
    s = " TAKES HOLD.";
    while (*s) msg_buf[i++] = *s++;
    if (g_sins >= 8) {
        s = " THE CASTLE SIGHS.";
        while (*s) msg_buf[i++] = *s++;
    }
    msg_buf[i] = 0;
}

static void confirm_sacrifice(void) {
    uint8_t v = sac_list[g_sac_sel];
    g_virtues &= (uint8_t)~(1 << v);
    g_sins++;
    if (g_pending_is_pact) {
        sfx_play(sfx_sacrifice);
    } else {
        /* Required gate: the sinister unlock sting, not the old
           two-tone sacrifice sound. */
        sfx_play(sfx_gate_sting);
    }
    if (g_pending_is_pact) {
        g_pacts |= (1 << g_pending_id);
        draw_room();
        show_message(pact_msgs[g_pending_id]);
    } else {
        g_gates |= (1 << g_pending_id);
        build_offer_msg(v);
        draw_room();
        show_message(msg_buf);
    }
}

static void do_ending(void) {
    const char *txt;
    uint8_t r;
    if (g_sins >= 8)
        txt = "YOU DO NOT LEAVE. THE THRONE WAS WAITING, AND SO WERE YOU. "
              "THE GATES OPEN FOR THE NEXT PILGRIM. YOU ARE THE KEEPER. "
              "DARK ENDING.";
    else if (g_sins >= 4)
        txt = "YOU ESCAPE, BUT THE CASTLE WALKS WITH YOU NOW. "
              "YOUR SHADOW SOMETIMES MOVES FIRST. YOU ARE FREE. MOSTLY. "
              "TAINTED ENDING.";
    else
        txt = "DAWN BREAKS OVER THE HILLS. YOU WALK OUT WITH YOUR SOUL "
              "MOSTLY YOUR OWN. THE CASTLE SHUTS ITS EYES BEHIND YOU. "
              "PURE ENDING.";
    wrap_message(txt);
    clear_screen();
    for (r = 0; r < msg_nlines && r < 17; r++)
        draw_str(0, r, msg_lines[r]);
    draw_str(0, 17, "PRESS START");
    /* The final screen gets its own composed theme, matched to the
       ending and in the same three-channel style as the title theme
       — major and resolving for pure, minor and unresolved for
       tainted, harmonic-minor and sinking for the keeper. The track
       loops until the player leaves the screen. */
    if (g_sins >= 8) {
        end_mel_cur = end_keeper_mel;
        end_harm_cur = end_keeper_harm;
        end_bass_cur = end_keeper_bass;
        g_end_track = 3;
    } else if (g_sins >= 4) {
        end_mel_cur = end_tainted_mel;
        end_harm_cur = end_tainted_harm;
        end_bass_cur = end_tainted_bass;
        g_end_track = 2;
    } else {
        end_mel_cur = end_pure_mel;
        end_harm_cur = end_pure_harm;
        end_bass_cur = end_pure_bass;
        g_end_track = 1;
    }
    mus_seq_cur = end_seq;
    mus_on = 1;
    mus_idx = 0;
    mus_timer = 0;
    end_mel_idx = 0; end_mel_timer = 0;
    end_harm_idx = 0; end_harm_timer = 0;
    end_bass_idx = 0; end_bass_timer = 0;
    g_state = ST_END;
    move_sprite(0, 200, 200);
}

static void interact(void) {
    const Hotspot *h = &hs_rooms[g_room][g_hot];
    switch (h->type) {
        case HS_LOOK:
            show_message(h->look);
            break;
        case HS_TAKE: {
            /* Round-3 fix: hide the sprite BEFORE the pickup message
               displays, so the player sees the item vanish as they read
               the message. The room redraw on text close keeps it gone
               (draw_room blanks taken items), and the wipe column draw
               does the same per column.
               Round-4 fix: record the take in g_taken (never cleared)
               as well as g_items, so consuming the item later cannot
               make it reappear in the room.
               Round-5: a TAKE lock (the prism's LK_CHART) denies the
               take while the key item is missing and consumes that
               item on a successful take — single use, like every
               other item. The 1.1 backtrack locks follow the same
               pattern: LK_STARKEY for the Moonstone, LK_DROWNEDKEY
               for the Blade. */
            uint8_t twisted = (g_sins >= 4);
            if (h->lock == LK_CHART && !item_has(IT_CHART)) {
                sfx_play(sfx_denied);
                show_message("THE PRISM IS HIDDEN AMONG THE TOMES. "
                             "A STAR CHART WOULD MARK ITS PLACE.");
                break;
            }
            if (h->lock == LK_STARKEY && !item_has(IT_STARKEY)) {
                sfx_play(sfx_denied);
                show_message("THE MOONSTONE IS SEALED. A STAR KEY "
                             "FROM THE SUNKEN CHAPEL OPENS IT.");
                break;
            }
            if (h->lock == LK_DROWNEDKEY && !item_has(IT_DROWNEDKEY)) {
                sfx_play(sfx_denied);
                show_message("THE BLADE IS LOCKED. A DROWNED KEY "
                             "FROM THE DROWNED WING OPENS IT.");
                break;
            }
            if (h->lock == LK_CHART) item_drop(IT_CHART);
            if (h->lock == LK_STARKEY) item_drop(IT_STARKEY);
            if (h->lock == LK_DROWNEDKEY) item_drop(IT_DROWNEDKEY);
            item_give(h->param);
            taken_set(h->param);
            set_bkg_tile_xy(h->x, h->y,
                twisted ? T_FLOOR_DARK : T_FLOOR);
            sfx_play(sfx_pickup);
            hs_move(1);
            show_message(take_msgs[h->param]);
            break;
        }
        case HS_DOOR:
            /* Round-4 fix: an item-locked door already passed stays
               open forever (g_locks_open), so the player can travel
               back through it without the consumed item — notably the
               ladder descent, which used to demand the ladder again
               after its first use consumed it. */
            if ((h->lock == LK_KEY || h->lock == LK_LADDER ||
                 h->lock == LK_OIL || h->lock == LK_OBSIDIAN ||
                 h->lock == LK_ICEKEY ||
                 h->lock == LK_HORNKEY) &&
                (g_locks_open & (uint16_t)(1u << h->lock))) {
                enter_room(h->param);
                break;
            }
            if (h->lock == LK_KEY && !(g_items & (uint16_t)(1u << IT_KEY))) {
                sfx_play(sfx_denied);
                show_message("LOCKED. A RUSTY KEY WOULD FIT.");
            } else if (h->lock == LK_LADDER &&
                       !(g_items & (uint16_t)(1u << IT_LADDER))) {
                sfx_play(sfx_denied);
                show_message("THE SHAFT IS DEEP. A LADDER WOULD REACH.");
            } else if (h->lock == LK_OIL &&
                       !(g_items & (uint16_t)(1u << IT_OIL))) {
                sfx_play(sfx_denied);
                show_message("THE PASSAGE DOOR IS RUSTED SHUT. OIL "
                             "WOULD LOOSEN IT.");
            } else if (h->lock == LK_OBSIDIAN &&
                       !(g_items & (uint16_t)(1u << IT_OBSIDIAN))) {
                sfx_play(sfx_denied);
                show_message("THE SANCTUM DOOR NEEDS A BLACK KEY.");
            } else if (h->lock == LK_ICEKEY &&
                       !item_has(IT_ICEKEY)) {
                sfx_play(sfx_denied);
                show_message("THE ICE DOOR IS FROZEN SHUT.");
            } else if (h->lock == LK_HORNKEY &&
                       !item_has(IT_LOFTKEY)) {
                sfx_play(sfx_denied);
                show_message("THE CHOIR LOFT IS LOCKED. A SMALL KEY "
                             "WOULD FIT. A PASSAGE WINDS DEEPER FROM "
                             "THIS HALL.");
            } else if (h->lock == LK_MIRROR && !g_mirror_used) {
                sfx_play(sfx_denied);
                show_message("THE GATE IS SEALED BEHIND BLACK GLASS. "
                             "SOMETHING THAT REFLECTS MIGHT REVEAL IT.");
            } else if (h->lock == LK_BELL && !g_bell_used) {
                sfx_play(sfx_denied);
                show_message("A STONE GUARDIAN BLOCKS THE WAY. "
                             "ONLY A PURE TONE COULD STIR IT.");
            } else if (h->lock == LK_LENS && !g_lens_used) {
                sfx_play(sfx_denied);
                show_message("THE LENS GATE IS DARK. FOCUS THE STAR "
                             "LENS FIRST.");
            } else if (h->lock == LK_ECHO && !g_echo_used) {
                sfx_play(sfx_denied);
                show_message("THE ECHO GATE SLEEPS. WAKE THE "
                             "GATEKEEPER FIRST.");
            } else if (h->lock == LK_GEM && !g_gem_used) {
                sfx_play(sfx_denied);
                show_message("THE HIDDEN DOOR IS SEALED. A PALE GEM WOULD OPEN IT.");
            } else if (h->lock == LK_RIDDLE && !g_riddle_used) {
                sfx_play(sfx_denied);
                show_message("THE RIDDLE GUARDIAN BARS THE WAY.");
            } else if (h->lock == LK_KNIGHT && !g_knight_used) {
                sfx_play(sfx_denied);
                show_message("THE DROWNED KNIGHT BARS THE WAY. ONLY A TRUE BLADE COULD MOVE IT.");
            } else if (h->lock == LK_BRIDGE && !g_bridge_built) {
                sfx_play(sfx_denied);
                show_message("THE CHASM NEEDS WOOD. A DEAD TREE "
                             "STANDS IN THE COURTYARD.");
            } else {
                /* Item locks consume their item on passing AND record
                   the door as opened: single use, enforced
                   mechanically, with the passage staying open. Flag
                   locks need no item. */
                if (h->lock == LK_KEY) {
                    g_items &= (uint16_t)~(1u << IT_KEY);
                    g_locks_open |= (uint16_t)(1u << LK_KEY);
                } else if (h->lock == LK_LADDER) {
                    g_items &= (uint16_t)~(1u << IT_LADDER);
                    g_locks_open |= (uint16_t)(1u << LK_LADDER);
                } else if (h->lock == LK_OIL) {
                    g_items &= (uint16_t)~(1u << IT_OIL);
                    g_locks_open |= (uint16_t)(1u << LK_OIL);
                } else if (h->lock == LK_OBSIDIAN) {
                    g_items &= (uint16_t)~(1u << IT_OBSIDIAN);
                    g_locks_open |= (uint16_t)(1u << LK_OBSIDIAN);
                } else if (h->lock == LK_HORNKEY) {
                    item_drop(IT_LOFTKEY);
                    g_locks_open |= (uint16_t)(1u << LK_HORNKEY);
                } else if (h->lock == LK_ICEKEY) {
                    item_drop(IT_ICEKEY);
                    g_locks_open |= (uint16_t)(1u << LK_ICEKEY);
                }
                enter_room(h->param);
            }
            break;
        case HS_EXIT:
            if (!item_has(IT_FINALKEY) || !item_has(IT_SEAL)) {
                sfx_play(sfx_denied);
                show_message("THE FINAL GATE NEEDS THE WHOLE KEY AND "
                             "THE KEEPER'S SEAL. JOIN THE KEY AT THE "
                             "FORGE, AND BEAR THE SEAL FROM "
                             "THE DROWNED WING.");
            } else {
                sfx_play(sfx_unlock);
                do_ending();
            }
            break;
        case HS_GATE: {
            uint8_t gid = h->param;
            if (g_gates & (1 << gid)) {
                enter_room(gate_target[gid]);
            } else if (gate_req[gid] == RQ_MIRROR && !g_mirror_used) {
                sfx_play(sfx_denied);
                show_message("THE GATE IS SEALED BEHIND BLACK GLASS. "
                             "SOMETHING THAT REFLECTS MIGHT REVEAL IT.");
            } else if (gate_req[gid] == RQ_BELL && !g_bell_used) {
                sfx_play(sfx_denied);
                show_message("A STONE GUARDIAN BLOCKS THE WAY. "
                             "ONLY A PURE TONE COULD STIR IT.");
            } else if (virtues_remaining() == 0) {
                sfx_play(sfx_denied);
                show_message("YOU HAVE NOTHING LEFT TO OFFER.");
            } else if (gid == 2 && !(g_virtues & (1 << 7))) {
                /* Hope already paid elsewhere: the seal recognizes
                   the Despair already in you, and opens for free. */
                g_gates |= (1 << 2);
                sfx_play(sfx_unlock);
                show_message("THE SEAL RECOGNIZES YOUR DESPAIR.");
            } else {
                g_pending_id = gid;
                g_pending_is_pact = 0;
                g_sac_sel = 0;
                build_sac_list();
                draw_sac_menu();
                g_state = ST_SAC;
                move_sprite(0, 200, 200);
            }
            break;
        }
        case HS_PACT: {
            uint8_t pid = h->param;
            if (g_pacts & (1 << pid)) {
                show_message("IT HAS NOTHING MORE TO SAY.");
            } else if (virtues_remaining() <= gates_remaining()) {
                sfx_play(sfx_denied);
                show_message("THE SPIRIT HESITATES. 'YOU CANNOT AFFORD "
                             "MY PRICE YET. THE CASTLE STILL DEMANDS "
                             "ITS TOLLS.'");
            } else {
                g_pending_id = pid;
                g_pending_is_pact = 1;
                g_sac_sel = 0;
                build_sac_list();
                draw_sac_menu();
                g_state = ST_SAC;
                move_sprite(0, 200, 200);
            }
            break;
        }
        case HS_USE:
            /* Every USE checks its used-flag FIRST (so a repeat visit
               after consumption shows the "already done" message, not a
               denied one), then its item. A denied USE consumes
               nothing. A successful USE consumes its item. */
            if (h->param == USE_MIRROR) {
                if (g_mirror_used) {
                    show_message("THE GLASS ALREADY SHOWS THE WAY.");
                } else if (!(g_items & (uint16_t)(1u << IT_MIRROR))) {
                    sfx_play(sfx_denied);
                    show_message("A WALL OF BLACK GLASS. IT SHOWS "
                                 "NOTHING BUT DARKNESS.");
                } else {
                    g_mirror_used = 1;
                    g_items &= (uint16_t)~(1u << IT_MIRROR);
                    sfx_play(sfx_unlock);
                    show_message("YOU HOLD THE MIRROR UP. THE GLASS "
                                 "SHIVERS AND SHOWS A HIDDEN GATE.");
                }
            } else if (h->param == USE_WELL) {
                if (g_well_used) {
                    show_message("THE WELL IS EMPTY NOW.");
                } else if (!(g_items & (uint16_t)(1u << IT_ROPE))) {
                    sfx_play(sfx_denied);
                    show_message("THE WELL IS DEEP AND DARK. SOMETHING "
                                 "HANGS WITHIN REACH. ROPE COULD "
                                 "FETCH IT. A PATIO OPENS OFF THIS "
                                 "COURTYARD.");
                } else {
                    g_well_used = 1;
                    g_items &= (uint16_t)~(1u << IT_ROPE);
                    g_items |= (uint16_t)(1u << IT_CANDLE);
                    sfx_play(sfx_pickup);
                    hs_move(1);
                    show_message("YOU LOWER THE ROPE AND HAUL UP A "
                                 "CANDLE, STILL BURNING.");
                }
            } else if (h->param == USE_LENS) {
                if (g_lens_used) {
                    show_message("THE LENS ALREADY FOCUSES THE LIGHT.");
                } else if (!(g_items & (uint16_t)(1u << IT_PRISM))) {
                    sfx_play(sfx_denied);
                    show_message("THE STAR LENS THIRSTS FOR A "
                                 "PRISM'S LIGHT.");
                } else {
                    g_lens_used = 1;
                    g_items &= (uint16_t)~(1u << IT_PRISM);
                    g_items |= (uint16_t)(1u << IT_SUNSTONE);
                    sfx_play(sfx_unlock);
                    show_message("YOU SET THE PRISM IN THE LENS. "
                                 "STARLIGHT POURS THROUGH AND THE GATE "
                                 "SHIVERS OPEN. A SUNSTONE FALLS FROM "
                                 "THE LENS, HOLDING DAWN LIGHT.");
                }
            } else if (h->param == USE_ECHO) {
                if (g_echo_used) {
                    show_message("THE GATEKEEPER HAS STEPPED ASIDE.");
                } else if (!(g_items & (uint16_t)(1u << IT_HORN))) {
                    sfx_play(sfx_denied);
                    show_message("THE GATEKEEPER SLEEPS SOUNDLY. ONLY "
                                 "A HORN'S BLAST COULD WAKE IT. A "
                                 "CHOIR LOFT OPENS OFF THIS HALL.");
                } else {
                    g_echo_used = 1;
                    g_items &= (uint16_t)~(1u << IT_HORN);
                    sfx_play(sfx_unlock);
                    show_message("YOU BLOW THE HORN. THE BLAST WAKES "
                                 "THE GATEKEEPER, AND IT STEPS ASIDE.");
                }
            } else if (h->param == USE_TREE) {
                if (g_tree_felled) {
                    show_message("THE STUMP OF THE DEAD TREE. ITS "
                                 "WOOD IS GONE.");
                } else if (!(g_items & (uint16_t)(1u << IT_AXE))) {
                    sfx_play(sfx_denied);
                    show_message("A DEAD TREE STANDS HERE. AN AXE "
                                 "COULD FELL IT.");
                } else {
                    g_tree_felled = 1;
                    g_items &= (uint16_t)~(1u << IT_AXE);
                    g_items |= (uint16_t)(1u << IT_WOOD);
                    sfx_play(sfx_unlock);
                    show_message("YOU FELL THE DEAD TREE. YOU TAKE "
                                 "ITS WOOD FOR THE CHASM.");
                }
            } else if (h->param == USE_WOOD) {
                if (g_bridge_built) {
                    show_message("THE BRIDGE STANDS.");
                } else if (!(g_items & (uint16_t)(1u << IT_WOOD))) {
                    sfx_play(sfx_denied);
                    show_message("THE CHASM NEEDS WOOD. A DEAD TREE "
                                 "STANDS IN THE COURTYARD.");
                } else {
                    g_bridge_built = 1;
                    g_items &= (uint16_t)~(1u << IT_WOOD);
                    sfx_play(sfx_unlock);
                    show_message("YOU LAY THE WOOD ACROSS THE CHASM. "
                                 "A BRIDGE.");
                }
            } else if (h->param == USE_FORGE) {
                if (g_items & (uint16_t)(1u << IT_FINALKEY)) {
                    show_message("THE FINAL KEY IS WHOLE.");
                } else if (!(g_items & (uint16_t)(1u << IT_BOW)) ||
                           !(g_items & (uint16_t)(1u << IT_STEM)) ||
                           !(g_items & (uint16_t)(1u << IT_SUNSTONE))) {
                    sfx_play(sfx_denied);
                    show_message("THE FORGE WAITS FOR THREE PIECES: "
                                 "BOW, STEM AND SUNSTONE.");
                } else {
                    g_items &= (uint16_t)~((1u << IT_BOW) |
                                           (1u << IT_STEM) |
                                           (1u << IT_SUNSTONE));
                    g_items |= (uint16_t)(1u << IT_FINALKEY);
                    sfx_play(sfx_unlock);
                    show_message("YOU JOIN BOW, STEM AND SUNSTONE ON "
                                 "THE ANVIL. THE FINAL KEY IS WHOLE.");
                }
            } else if (h->param == USE_COFFER) {
                if (g_coffer_used) {
                    show_message("THE COFFER IS EMPTY.");
                } else if (!(g_items & (uint16_t)(1u << IT_CANDLE))) {
                    sfx_play(sfx_denied);
                    show_message("A COFFER SEALED WITH WAX. FLAME "
                                 "WOULD MELT IT.");
                } else {
                    g_coffer_used = 1;
                    g_items &= (uint16_t)~(1u << IT_CANDLE);
                    g_items |= (uint16_t)(1u << IT_OBSIDIAN);
                    sfx_play(sfx_pickup);
                    show_message("YOU HOLD THE CANDLE TO THE WAX SEAL. "
                                 "IT MELTS. INSIDE: AN OBSIDIAN KEY.");
                }
            } else if (h->param == USE_POOL) {
                if (g_pool_melted) {
                    show_message("THE POOL IS MELTED AND EMPTY.");
                } else if (!item_has(IT_TORCH)) {
                    sfx_play(sfx_denied);
                    show_message("THE POOL IS FROZEN SOLID. A TORCH COULD MELT IT.");
                } else {
                    g_pool_melted = 1;
                    item_drop(IT_TORCH);
                    item_give(IT_ICEKEY);
                    sfx_play(sfx_unlock);
                    show_message("YOU HOLD THE TORCH TO THE ICE. IT MELTS. INSIDE: AN ICE KEY.");
                }
            } else if (h->param == USE_GEMSLOT) {
                if (g_gem_used) {
                    show_message("THE SLOT ALREADY HOLDS ITS GEM.");
                } else if (!item_has(IT_GEM)) {
                    sfx_play(sfx_denied);
                    show_message("A SLOT THE SIZE OF A PALE GEM. THE GALLERY OFF THIS HALL HOLDS ONE.");
                } else {
                    g_gem_used = 1;
                    item_drop(IT_GEM);
                    sfx_play(sfx_unlock);
                    show_message("YOU SET THE PALE GEM IN THE SLOT. THE HIDDEN DOOR GRINDS OPEN.");
                }
            } else if (h->param == USE_RIDDLE) {
                if (g_riddle_used) {
                    show_message("THE GUARDIAN HAS STEPPED ASIDE.");
                } else if (!item_has(IT_MOONSTONE)) {
                    sfx_play(sfx_denied);
                    show_message("THE GUARDIAN WAITS FOR AN ANSWER THAT GLOWS WITHOUT FIRE. THE OBSERVATORY HOLDS IT.");
                } else {
                    g_riddle_used = 1;
                    item_drop(IT_MOONSTONE);
                    sfx_play(sfx_unlock);
                    show_message("YOU HOLD UP THE MOONSTONE. IT GLOWS WITHOUT FIRE. THE GUARDIAN STEPS ASIDE.");
                }
            } else if (h->param == USE_KNIGHT) {
                if (g_knight_used) {
                    show_message("THE KNIGHT HAS STEPPED ASIDE.");
                } else if (!item_has(IT_BLADE)) {
                    sfx_play(sfx_denied);
                    show_message("THE KNIGHT DOES NOT MOVE. ONLY A TRUE BLADE COULD MAKE IT STEP ASIDE.");
                } else {
                    g_knight_used = 1;
                    item_drop(IT_BLADE);
                    sfx_play(sfx_unlock);
                    show_message("YOU RAISE THE SILVER BLADE. THE KNIGHT STEPS ASIDE.");
                }
            } else {  /* USE_BELL: the stair guardian */
                if (g_bell_used) {
                    show_message("THE GUARDIAN HAS STEPPED ASIDE.");
                } else if (!(g_items & (uint16_t)(1u << IT_BELL))) {
                    sfx_play(sfx_denied);
                    show_message("A STONE GUARDIAN BLOCKS THE STAIR. "
                                 "ITS EARS ARE STOPPED WITH WAX. ONLY THE "
                                 "SILVER BELL'S TONE COULD STIR IT.");
                } else {
                    g_bell_used = 1;
                    g_items &= (uint16_t)~(1u << IT_BELL);
                    sfx_play(sfx_unlock);
                    show_message("YOU RING THE BELL. THE TONE IS PURE. "
                                 "THE GUARDIAN STEPS ASIDE.");
                }
            }
            break;
    }
}

static void draw_inv(void) {
    uint8_t i, row, stage, n = 0;
    clear_screen();
    /* 1.1 layout for up to 26 items (held count stays well under
       this in practice, as most items are consumed on the path):
       the corruption portrait sits top-left (2x2 tiles); items fill
       two columns of 8 (rows 1-8); virtues and sins each get exactly
       8 rows (10-17), so no list ever clips off the bottom. */
    stage = (g_sins >= 7) ? 3 : (g_sins >= 5) ? 2 : (g_sins >= 2) ? 1 : 0;
    set_bkg_tile_xy(0, 0, PORT_BASE + stage * 4 + 0);
    set_bkg_tile_xy(1, 0, PORT_BASE + stage * 4 + 1);
    set_bkg_tile_xy(0, 1, PORT_BASE + stage * 4 + 2);
    set_bkg_tile_xy(1, 1, PORT_BASE + stage * 4 + 3);
    draw_str(3, 0, "ITEMS");
    draw_str(12, 0, "B: BACK");
    for (i = 0; i < 26; i++)
        if (item_has(i)) {
            if (n < 8) draw_str(2, 1 + n, item_short_names[i]);
            else draw_str(10, 1 + n - 8, item_short_names[i]);
            n++;
        }
    if (n == 0) draw_str(2, 1, "NOTHING YET.");
    draw_str(2, 9, "VIRTUES");
    draw_str(12, 9, "SINS");
    row = 10;
    for (i = 0; i < 8; i++)
        if (g_virtues & (1 << i)) {
            draw_str(2, row, virtue_names[i]);
            row++;
        }
    row = 10;
    for (i = 0; i < 8; i++)
        if (!(g_virtues & (1 << i))) {
            draw_str(12, row, sin_names[i]);
            row++;
        }
    if (row == 10) draw_str(12, 10, "NONE.");
}

static void draw_title(void) {
    uint8_t x, y;
    for (y = 0; y < 12; y++)
        for (x = 0; x < 20; x++)
            set_bkg_tile_xy(x, y,
                char_tile(room_maps[TITLE_MAP_INDEX][y][x], 0));
    for (x = 0; x < 20; x++) set_bkg_tile_xy(x, 12, T_DIV);
    for (y = 13; y < 18; y++)
        for (x = 0; x < 20; x++) set_bkg_tile_xy(x, y, T_BLANK);
    draw_str(6, 13, "SEVENFOLD");
    draw_str(4, 14, "PRESS START");
    draw_str(4, 15, "1-BIT JAM 15");
    draw_str(0, 16, "D-PAD: CHOOSE A: ACT");
    move_sprite(0, 200, 200);
    /* Title screen owns the music: restart the title theme here. */
    mus_seq_cur = music_seq;
    mus_on = 1;
    mus_idx = 0;
    mus_timer = 0;
    tit_mel_idx = 0; tit_mel_timer = 0;
    tit_harm_idx = 0; tit_harm_timer = 0;
    tit_bass_idx = 0; tit_bass_timer = 0;
}

static void reset_game(void) {
    g_sins = 0;
    g_items = 0;
    g_items2 = 0;
    g_taken = 0;
    g_taken2 = 0;
    g_end_track = 0;
    g_locks_open = 0;
    g_gates = 0;
    g_pacts = 0;
    g_mirror_used = 0;
    g_bell_used = 0;
    g_lens_used = 0;
    g_echo_used = 0;
    g_bridge_built = 0;
    g_well_used = 0;
    g_coffer_used = 0;
    g_tree_felled = 0;
    g_pool_melted = 0;
    g_gem_used = 0;
    g_riddle_used = 0;
    g_knight_used = 0;
    g_virtues = 0xFF;
    g_room = 0;
    g_hot = 0;
    g_frame = 0;
}

void main(void) {
    uint8_t pad, prev = 0, newpad;
    set_bkg_data(0, SCENE_COUNT, scene_tiles);
    set_bkg_data(FONT_BASE, FONT_COUNT, font_tiles);
    set_bkg_data(PORT_BASE, PORT_COUNT, portrait_tiles);
    set_sprite_data(SPR_CURSOR_TILE, 1, cursor_tile);
    set_sprite_tile(0, SPR_CURSOR_TILE);
    BGP_REG = 0xE4;
    OBP0_REG = 0xFC;
    /* Sound on: title music on channels 1+2+3, SFX on channel 1,
       all routed to both speakers. */
    NR52_REG = 0x80;
    NR51_REG = 0x77;
    NR50_REG = 0x77;
    /* Load the triangle wave table for the title bass. */
    { uint8_t w; for (w = 0; w < 16; w++) AUD3WAVE[w] = bass_wave[w]; }
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
    reset_game();
    draw_title();
    g_state = ST_TITLE;
    while (1) {
        wait_vbl_done();
        g_frame++;
        music_tick();
        sfx_tick();
        pad = joypad();
        newpad = pad & (uint8_t)~prev;
        prev = pad;
        switch (g_state) {
            case ST_TITLE:
                if (newpad & J_START) {
                    /* Music stops dead when the game starts: silence
                       all three title channels, not just the melody. */
                    mus_on = 0;
                    NR22_REG = 0x00;
                    NR12_REG = 0x00;
                    NR32_REG = 0x00;
                    reset_game();
                    enter_room(0);
                    show_message("YOU ENTER THE CASTLE. THE GATE SHUTS "
                                 "BEHIND YOU. SOMEWHERE ABOVE, A BELL "
                                 "TOLLS. FIND THE WAY OUT. AND MIND "
                                 "WHAT IT COSTS.");
                }
                break;
            case ST_ROOM:
                if (newpad & (J_LEFT | J_UP)) hs_move(-1);
                else if (newpad & (J_RIGHT | J_DOWN)) hs_move(1);
                if (newpad & J_A) { interact(); }
                if (newpad & J_START) {
                    draw_inv();
                    g_state = ST_INV;
                    move_sprite(0, 200, 200);
                }
                if (g_state == ST_ROOM) {
                    if (g_frame & 16) move_sprite(0, 200, 200);
                    else place_cursor();
                }
                break;
            case ST_TEXT:
                if (newpad & J_A) {
                    if ((uint8_t)(msg_page + 1) * 5 < msg_nlines) {
                        msg_page++;
                        draw_msg_page();
                    } else {
                        g_state = ST_ROOM;
                        draw_room();
                    }
                } else if (newpad & J_B) {
                    g_state = ST_ROOM;
                    draw_room();
                }
                break;
            case ST_SAC:
                if (newpad & J_UP) {
                    if (g_sac_sel > 0) {
                        g_sac_sel--;
                        draw_sac_menu();
                    }
                } else if (newpad & J_DOWN) {
                    if (g_sac_sel + 1 < sac_count) {
                        g_sac_sel++;
                        draw_sac_menu();
                    }
                } else if (newpad & J_A) {
                    confirm_sacrifice();
                } else if (newpad & J_B) {
                    g_state = ST_ROOM;
                    draw_room();
                }
                break;
            case ST_INV:
                if ((newpad & J_B) || (newpad & J_START)) {
                    g_state = ST_ROOM;
                    draw_room();
                }
                break;
            case ST_END:
                if ((newpad & J_A) || (newpad & J_START)) {
                    draw_title();
                    g_state = ST_TITLE;
                }
                break;
        }
    }
}
