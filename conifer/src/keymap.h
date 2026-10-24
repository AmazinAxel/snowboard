// Snowlayer keymap — edit this file to change your layout. Nothing else.
//
// Physical layout (42 keys, 3x6 per half + 3 thumbs per half):
//
//   ,-----------------------------.        ,-----------------------------.
//   |     L00 L01 L02 L03 L04 L05 |        | R00 R01 R02 R03 R04 R05     |
//   |     L10 L11 L12 L13 L14 L15 |        | R10 R11 R12 R13 R14 R15     |
//   |     L20 L21 L22 L23 L24 L25 |        | R20 R21 R22 R23 R24 R25     |
//   `---------------. L30 L31 L32 |        | R30 R31 R32 .---------------'
//                   `-------------'        `-------------'
//
// Keycodes: use the KC_* names from keycodes.h. KC_NO = dead key.
// LAYER(n) on a key = momentary layer n while held (like QMK's MO(n)).
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
#define NUM_LAYERS 3

// Layer indices, for readability in LAYER().
enum { BASE = 0, SYM = 1, NUM = 2 };

// ---------------------------------------------------------------------------
// The keymap. One block per layer, laid out exactly like the physical board.
//
// KEYMAP[layer][row][col] — rows/cols are the electrical matrix; the macro
// below maps a human-readable layout into it, so you only ever type the visual
// arrangement.
//
// Both halves are in the same column order: the right half is NOT reversed,
// and each half is written here left to right as you read it on the board.
//
// Do not add a column flip for the right half. That has been tried twice, and
// verified on hardware it makes the right half type mirrored (hjkl;' comes out
// as ';lkjh) — a clean col <-> 5-col permutation. One of those attempts went in
// while the matrix was dead from an unrelated bug, so it was never tested; don't
// trust a keymap change that hasn't been typed on a working board.
// ---------------------------------------------------------------------------

// The one exception is the right thumb row: it is wired COL5..COL3 left to
// right, so ROW7 takes R32 R31 R30. Verified on hardware (Space and Alt came
// out swapped without it).
//
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
  /* ROW7 */ { KC_NO, KC_NO, KC_NO, R32, R31, R30 } }

static const uint8_t KEYMAP[NUM_LAYERS][8][6] = {

  // BASE — QWERTY alphas, unmodified, so every game bind works as shipped.
  //
  // Every modifier is a plain held modifier, like a normal keyboard: it is
  // down exactly as long as the key is. Games hold Shift to sprint and Ctrl to
  // crouch, and anything cleverer than that breaks them.
  //
  //   ,---------------------------------.   ,---------------------------------.
  //   | Tab  Q    W    E    R    T      |   | Y    U    I    O    P    Bksp   |
  //   | Ctl  A    S    D    F    G      |   | H    J    K    L    ;    '      |
  //   | Sft  Z    X    C    V    B      |   | N    M    ,    .    /    Enter  |
  //   `-----------. Spc  SYM  Gui       |   | Spc  NUM  Alt  .----------------'
  //
  // Esc is not on BASE — it lives on the Gui thumb under SYM and at L00 under
  // NUM.
  [BASE] = LAYOUT(
    KC_TAB,  KC_Q, KC_W, KC_E, KC_R, KC_T,      KC_Y, KC_U, KC_I,    KC_O,   KC_P,    KC_BSPC,
    KC_LCTL, KC_A, KC_S, KC_D, KC_F, KC_G,      KC_H, KC_J, KC_K,    KC_L,   KC_SCLN, KC_QUOT,
    KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B,      KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_ENT,
             KC_SPC,  LAYER(SYM), KC_LGUI,      KC_SPC,  LAYER(NUM), KC_LALT),

  // SYM — symbols under the home row.
  //
  // The point of the whole layer: on a normal board every bracket, operator
  // and quote is either a pinky stretch to the far column or a Shift chord
  // with the number row. Here the left hand holds SYM and the right hand types
  // them from home position.
  //
  // Right hand home row is the pairs — ( ) { } — because those are what code
  // is made of. Operators sit on the row above, the rarer punctuation below.
  // Left hand gets the modifiers on its home row so Ctrl+symbol and
  // Alt+symbol still chord without leaving the layer.
  //
  //   ,---------------------------------.   ,---------------------------------.
  //   | ~    !    @    #    $    %      |   | ^    &    *    -    +    Del    |
  //   | Ctl  Gui  Alt  Sft  Ctl  |      |   | (    )    {    }    =    `      |
  //   | Sft  \    :    <    >    ?      |   | [    ]    _    "    /    Enter  |
  //   `-----------. Spc  ---- Esc       |   | Spc  ---- Alt  .----------------'
  //
  // Esc replaces Gui on the inner thumb here; Gui is still on the home row.
  //
  // S(k) sends Shift+k (from keycodes.h), so the glyphs come out as drawn
  // above rather than as their unshifted base keys.
  [SYM] = LAYOUT(
    S(KC_GRV), S(KC_1),    S(KC_2),    S(KC_3),    S(KC_4),   S(KC_5),       S(KC_6),  S(KC_7),  S(KC_8),    KC_MINS,    S(KC_EQL), KC_DEL,
    KC_TRNS,   KC_LGUI,    KC_LALT,    KC_LSFT,    KC_LCTL,   S(KC_BSLS),    S(KC_9),  S(KC_0),  S(KC_LBRC), S(KC_RBRC), KC_EQL,    KC_GRV,
    KC_TRNS,   KC_BSLS,    S(KC_SCLN), S(KC_COMM), S(KC_DOT), S(KC_SLSH),    KC_LBRC,  KC_RBRC,  S(KC_MINS), S(KC_QUOT), KC_SLSH,   KC_TRNS,
               KC_TRNS,    KC_TRNS,    KC_ESC,                               KC_TRNS,  KC_TRNS,  KC_TRNS),

  // NUM — numpad right, arrows and function keys left.
  //
  // There is no number row on a 42-key board and this is the replacement: the
  // right hand holds NUM and types digits from a 3x3 cluster under home
  // position, which beats a number row even on a full-size board. 0 sits on
  // the thumb-adjacent slot next to 1.
  //
  // Arrows go on the left in the vim positions transposed to that hand, so the
  // right hand — the one holding the layer — never has to do both jobs. F1-F5
  // sit above the arrows and F6-F10 below; F11 and F12 take the inner keys.
  //
  //   ,---------------------------------.   ,---------------------------------.
  //   | Esc  F1   F2   F3   F4   F5     |   | F12   7    8    9    -    Bksp  |
  //   | Ctl  Left Down Up   Rght F11    |   | *     4    5    6    =    Enter |
  //   | Sft  F6   F7   F8   F9   F10    |   | /     1    2    3    .    ----  |
  //   `-----------. Spc  ---- Gui       |   | 0     ---- Alt  .----------------'
  [NUM] = LAYOUT(
    KC_ESC,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,      KC_F12,  KC_7, KC_8, KC_9, KC_MINS, KC_TRNS,
    KC_TRNS, KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, KC_F11,     KC_PAST, KC_4, KC_5, KC_6, KC_EQL,  KC_TRNS,
    KC_TRNS, KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,     KC_PSLS, KC_1, KC_2, KC_3, KC_DOT,  KC_NO,
             KC_TRNS, KC_TRNS, KC_TRNS,                      KC_0,    KC_TRNS, KC_TRNS),
};
// clang-format on
