// Snowlayer keymap — edit this file to change your layout. Nothing else.
//
// Physical layout (42 keys, 3x6 per half + 3 thumbs per half):
//
//   ,-----------------------------.        ,-----------------------------.
//   | L00 L01 L02 L03 L04 L05     |        | R00 R01 R02 R03 R04 R05     |
//   | L10 L11 L12 L13 L14 L15     |        | R10 R11 R12 R13 R14 R15     |
//   | L20 L21 L22 L23 L24 L25     |        | R20 R21 R22 R23 R24 R25     |
//   `-------------. L30 L31 L32   |        | R30 R31 R32 .---------------'
//                 `---------------'        `-------------'
//
// Keycodes: use the KC_* names from keycodes.h. KC_NO = dead key.
// LAYER(n) on a key = momentary layer n while held (like QMK's MO(n)).
// KC_OSFT / KC_OCTL / KC_OALT / KC_OGUI are one-shot modifiers: tap one, then
// tap a key, and the modifier applies to that key only. Holding one still acts
// as a plain held modifier.
//
// Modifiers get their own keys — no mod-tap, no tap dance. Both need a timing
// window to decide what a key meant, and a timing window is something to
// mistime. That is what the six thumb keys are for.
//
// Design notes, so a future edit does not undo them by accident:
//
//   * Alphas are QWERTY. Not because QWERTY is good — it is not, its
//     same-finger bigram rate is about three times an optimised layout's — but
//     because WASD and ZXCV have to stay where games expect them, and any
//     layout worth switching to puts high-frequency letters in exactly those
//     positions. There is no gaming layer here and deliberately so: nothing to
//     switch, nothing to forget to switch back.
//
//   * The ergonomic win therefore comes from everything *except* the alphas:
//     modifiers on thumbs instead of pinkies, symbols under the home row
//     instead of stretched to the number row, no number row at all, and arrows
//     under the right hand rather than out in a corner.
//
//   * Every layer key is a thumb, and every layer is reachable without the
//     hand leaving home position.
#pragma once

#include "keycodes.h"

// Number of layers. Keep this in sync with the KEYMAP array below.
#define NUM_LAYERS 4

// Layer indices, for readability in LAYER().
//
// ADJUST is entered by holding SYM and NUM together — it has no key of its
// own. resolveKey() searches from the highest layer down, so a position that
// is live on ADJUST wins over the same position on SYM or NUM.
enum { BASE = 0, SYM = 1, NUM = 2, ADJUST = 3 };

// ---------------------------------------------------------------------------
// The keymap. One block per layer, laid out exactly like the physical board.
//
// KEYMAP[layer][row][col] — rows/cols are the electrical matrix; the macro
// below maps a human-readable layout into it, so you only ever type the visual
// arrangement.
//
// Both halves are in the same column order: COL0 is the inner column on each
// hand, COL5 the outer. The right half is NOT electrically mirrored.
//
// This macro used to reverse the right half's columns, on the belief that the
// right hand's COL0 was its outer column. It is not — the symptom was that the
// whole right half typed mirrored (n->Enter, m->/, ,->., h->', j->;, k->l),
// which is a clean col <-> 5-col permutation and the signature of applying a
// flip to a matrix that was already in order.
// ---------------------------------------------------------------------------

// clang-format off
#define LAYOUT( \
    L00, L01, L02, L03, L04, L05,   R00, R01, R02, R03, R04, R05, \
    L10, L11, L12, L13, L14, L15,   R10, R11, R12, R13, R14, R15, \
    L20, L21, L22, L23, L24, L25,   R20, R21, R22, R23, R24, R25, \
                   L30, L31, L32,   R30, R31, R32                 ) \
{ /* ROW0 */ { L00, L01, L02, L03, L04, L05 },   \
  /* ROW1 */ { L10, L11, L12, L13, L14, L15 },   \
  /* ROW2 */ { L20, L21, L22, L23, L24, L25 },   \
  /* ROW3 */ { KC_NO, KC_NO, KC_NO, L30, L31, L32 }, \
  /* ROW4 */ { R00, R01, R02, R03, R04, R05 },   \
  /* ROW5 */ { R10, R11, R12, R13, R14, R15 },   \
  /* ROW6 */ { R20, R21, R22, R23, R24, R25 },   \
  /* ROW7 */ { KC_NO, KC_NO, KC_NO, R30, R31, R32 } }

static const uint8_t KEYMAP[NUM_LAYERS][8][6] = {

  // BASE — QWERTY alphas, unmodified, so every game bind works as shipped.
  //
  // The outer left column keeps Tab/Ctrl/Shift as *held* modifiers rather than
  // one-shots. Games hold Shift to sprint and Ctrl to crouch, and a one-shot
  // that clears after the next keypress cannot do that. The one-shot versions
  // live on the thumbs, which is where typing should reach for them.
  //
  //   ,---------------------------------.   ,---------------------------------.
  //   | Tab  Q    W    E    R    T      |   | Y    U    I    O    P    Bksp   |
  //   | Ctl  A    S    D    F    G      |   | H    J    K    L    ;    '      |
  //   | Sft  Z    X    C    V    B      |   | N    M    ,    .    /    Enter  |
  //   `-----------. Esc  SYM  Spc       |   | Sft* Ctl* Alt* .----------------'
  [BASE] = LAYOUT(
    KC_TAB,  KC_Q, KC_W, KC_E, KC_R, KC_T,      KC_Y, KC_U, KC_I,    KC_O,   KC_P,    KC_BSPC,
    KC_LCTL, KC_A, KC_S, KC_D, KC_F, KC_G,      KC_H, KC_J, KC_K,    KC_L,   KC_SCLN, KC_QUOT,
    KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B,      KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_ENT,
             KC_ESC, LAYER(SYM), KC_SPC,        KC_OSFT, LAYER(NUM), KC_OALT),

  // SYM — symbols under the home row.
  //
  // The point of the whole layer: on a normal board every bracket, operator
  // and quote is either a pinky stretch to the far column or a Shift chord
  // with the number row. Here the left hand holds SYM and the right hand types
  // them from home position.
  //
  // Right hand home row is the pairs — ( ) { } — because those are what code
  // is made of. Operators sit on the row above, the rarer punctuation below.
  // Left hand gets the one-shot modifiers on its home row so Ctrl+symbol and
  // Alt+symbol still chord without leaving the layer.
  //
  //   ,---------------------------------.   ,---------------------------------.
  //   | ~    !    @    #    $    %      |   | ^    &    *    -    +    Del    |
  //   | Ctl  Gui* Alt* Sft* Ctl* |      |   | (    )    {    }    =    `      |
  //   | Sft  \    :    <    >    ?      |   | [    ]    _    "    /    Enter  |
  //   `-----------. Esc  ---- Spc       |   | Sft* ADJ  Alt* .----------------'
  //
  // The shifted characters here are plain keycodes — the firmware sends the
  // usage ID and the host's US layout applies Shift. Anything needing Shift is
  // written as the base key with KC_LSFT folded in by the host, which this
  // firmware cannot express, so the genuinely shifted glyphs (! @ # $ % ^ & *
  // etc.) are reached with the one-shot Shift on the left home row instead.
  // ponytail: no shifted-keycode encoding; add one if reaching for Shift on
  // this layer actually annoys you in practice.
  [SYM] = LAYOUT(
    KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,       KC_6,    KC_7,    KC_8,    KC_MINS, KC_EQL,  KC_DEL,
    KC_TRNS, KC_OGUI, KC_OALT, KC_OSFT, KC_OCTL, KC_BSLS,    KC_9,    KC_0,    KC_LBRC, KC_RBRC, KC_EQL,  KC_GRV,
    KC_TRNS, KC_BSLS, KC_SCLN, KC_COMM, KC_DOT,  KC_SLSH,    KC_LBRC, KC_RBRC, KC_MINS, KC_QUOT, KC_SLSH, KC_TRNS,
             KC_TRNS, KC_TRNS, KC_TRNS,                      KC_TRNS, LAYER(ADJUST), KC_TRNS),

  // NUM — numpad right, navigation left.
  //
  // There is no number row on a 42-key board and this is the replacement: the
  // right hand holds NUM and types digits from a 3x3 cluster under home
  // position, which beats a number row even on a full-size board. 0 sits on
  // the thumb-adjacent slot next to 1.
  //
  // Arrows go on the left in the vim positions transposed to that hand, so the
  // right hand — the one holding the layer — never has to do both jobs.
  //
  //   ,---------------------------------.   ,---------------------------------.
  //   | Esc  Home PgDn PgUp End  ----   |   | ----  7    8    9    -    Bksp  |
  //   | Ctl  Left Down Up   Rght ----   |   | *     4    5    6    +    Enter |
  //   | Sft  ---- ---- ---- ---- ----   |   | /     1    2    3    .    ----  |
  //   `-----------. Esc  ADJ  Spc       |   | 0     ---- Alt* .----------------'
  [NUM] = LAYOUT(
    KC_ESC,  KC_HOME, KC_PGDN, KC_PGUP, KC_END,  KC_NO,      KC_NO,   KC_7, KC_8, KC_9, KC_MINS, KC_TRNS,
    KC_TRNS, KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, KC_NO,      KC_PAST, KC_4, KC_5, KC_6, KC_EQL,  KC_TRNS,
    KC_TRNS, KC_NO,   KC_INS,  KC_NO,   KC_NO,   KC_NO,      KC_PSLS, KC_1, KC_2, KC_3, KC_DOT,  KC_TRNS,
             KC_TRNS, LAYER(ADJUST), KC_TRNS,                KC_0,    KC_TRNS, KC_TRNS),

  // ADJUST — function keys and the rest. Reached by holding both thumb layer
  // keys, so it costs no key of its own.
  //
  //   ,---------------------------------.   ,---------------------------------.
  //   | ---- F1   F2   F3   F4   F5     |   | F6   F7   F8   F9   F10  ----   |
  //   | Ctl  ---- ---- ---- ---- ----   |   | ---- ---- ---- F11  F12  ----   |
  //   | Sft  ---- ---- ---- ---- ----   |   | ---- ---- ---- ---- ---- Caps   |
  //   `-----------. ---- ---- ----      |   | ---- ---- PScr .----------------'
  [ADJUST] = LAYOUT(
    KC_NO,   KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,      KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_NO,
    KC_TRNS, KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,      KC_NO,   KC_NO,   KC_NO,   KC_F11,  KC_F12,  KC_NO,
    KC_TRNS, KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,      KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_CAPS,
             KC_NO,   KC_TRNS, KC_NO,                        KC_NO,   KC_TRNS, KC_PSCR),
};
// clang-format on
