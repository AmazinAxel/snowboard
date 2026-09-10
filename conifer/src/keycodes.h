// USB HID usage IDs (HID Usage Tables 1.12, keyboard page 0x07).
// Full US QWERTY set. These are raw usage codes, not ASCII — the firmware
// sends them straight into the HID report, so the host applies the US layout.
#pragma once

#include <stdint.h>

#define KC_NO   0x00  // dead key
#define KC_TRNS 0x01  // transparent: fall through to the layer below

// Letters
#define KC_A 0x04
#define KC_B 0x05
#define KC_C 0x06
#define KC_D 0x07
#define KC_E 0x08
#define KC_F 0x09
#define KC_G 0x0A
#define KC_H 0x0B
#define KC_I 0x0C
#define KC_J 0x0D
#define KC_K 0x0E
#define KC_L 0x0F
#define KC_M 0x10
#define KC_N 0x11
#define KC_O 0x12
#define KC_P 0x13
#define KC_Q 0x14
#define KC_R 0x15
#define KC_S 0x16
#define KC_T 0x17
#define KC_U 0x18
#define KC_V 0x19
#define KC_W 0x1A
#define KC_X 0x1B
#define KC_Y 0x1C
#define KC_Z 0x1D

// Numbers (top row)
#define KC_1 0x1E
#define KC_2 0x1F
#define KC_3 0x20
#define KC_4 0x21
#define KC_5 0x22
#define KC_6 0x23
#define KC_7 0x24
#define KC_8 0x25
#define KC_9 0x26
#define KC_0 0x27

// Punctuation / control
#define KC_ENT  0x28  // Enter
#define KC_ESC  0x29
#define KC_BSPC 0x2A  // Backspace
#define KC_TAB  0x2B
#define KC_SPC  0x2C  // Space
#define KC_MINS 0x2D  // - _
#define KC_EQL  0x2E  // = +
#define KC_LBRC 0x2F  // [ {
#define KC_RBRC 0x30  // ] }
#define KC_BSLS 0x31  // \ |
#define KC_NUHS 0x32  // non-US # ~
#define KC_SCLN 0x33  // ; :
#define KC_QUOT 0x34  // ' "
#define KC_GRV  0x35  // ` ~
#define KC_COMM 0x36  // , <
#define KC_DOT  0x37  // . >
#define KC_SLSH 0x38  // / ?
#define KC_CAPS 0x39  // Caps Lock

// Function keys
#define KC_F1  0x3A
#define KC_F2  0x3B
#define KC_F3  0x3C
#define KC_F4  0x3D
#define KC_F5  0x3E
#define KC_F6  0x3F
#define KC_F7  0x40
#define KC_F8  0x41
#define KC_F9  0x42
#define KC_F10 0x43
#define KC_F11 0x44
#define KC_F12 0x45

// Navigation / editing
#define KC_PSCR 0x46  // Print Screen
#define KC_SLCK 0x47  // Scroll Lock
#define KC_PAUS 0x48
#define KC_INS  0x49
#define KC_HOME 0x4A
#define KC_PGUP 0x4B
#define KC_DEL  0x4C
#define KC_END  0x4D
#define KC_PGDN 0x4E
#define KC_RGHT 0x4F
#define KC_LEFT 0x50
#define KC_DOWN 0x51
#define KC_UP   0x52

// Keypad
#define KC_NLCK 0x53  // Num Lock
#define KC_PSLS 0x54  // keypad /
#define KC_PAST 0x55  // keypad *
#define KC_PMNS 0x56  // keypad -
#define KC_PPLS 0x57  // keypad +
#define KC_PENT 0x58  // keypad Enter
#define KC_P1   0x59
#define KC_P2   0x5A
#define KC_P3   0x5B
#define KC_P4   0x5C
#define KC_P5   0x5D
#define KC_P6   0x5E
#define KC_P7   0x5F
#define KC_P8   0x60
#define KC_P9   0x61
#define KC_P0   0x62
#define KC_PDOT 0x63  // keypad .
#define KC_NUBS 0x64  // non-US \ |
#define KC_APP  0x65  // Application / Menu

// Modifiers.
// Encoded as 0xE0..0xE7 (their real HID usage IDs); the scanner turns these
// into the report's modifier bitmask rather than a key slot.
#define KC_LCTL 0xE0
#define KC_LSFT 0xE1
#define KC_LALT 0xE2
#define KC_LGUI 0xE3
#define KC_RCTL 0xE4
#define KC_RSFT 0xE5
#define KC_RALT 0xE6
#define KC_RGUI 0xE7

#define IS_MOD(k) ((k) >= KC_LCTL && (k) <= KC_RGUI)
#define MOD_BIT(k) (1 << ((k) - KC_LCTL))

// One-shot ("sticky") modifiers, 0xE8..0xEF — the same order as 0xE0..0xE7,
// with bit 3 set. 0xE8-0xEF is unused by the HID keyboard page, so this can
// never collide with a real usage ID.
//
// Tap one, then tap a key: the modifier applies to that key and then clears.
// This replaces holding a modifier down, which is the reach these thumb keys
// exist to remove. Unlike a mod-tap or a tap-dance there is no timing window
// involved, so there is nothing to mistime at speed.
//
// Holding one still works as a plain modifier, so Ctrl+drag and game binds
// that expect a held Shift behave normally.
#define OSM(mod) ((mod) | 0x08)
#define IS_OSM(k) ((k) >= 0xE8 && (k) <= 0xEF)
#define OSM_BIT(k) (1 << ((k) - 0xE8))

#define KC_OSFT OSM(KC_LSFT)
#define KC_OCTL OSM(KC_LCTL)
#define KC_OALT OSM(KC_LALT)
#define KC_OGUI OSM(KC_LGUI)

// Momentary layer switch. 0xF0 | layer — outside the HID usage range we use,
// so it can never collide with a real keycode.
#define LAYER(n) (0xF0 | (n))
#define IS_LAYER(k) (((k) & 0xF0) == 0xF0)
#define LAYER_NUM(k) ((k) & 0x0F)
