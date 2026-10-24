# Snowlayer

42-key split ergo keyboard: custom PCB, printed case, own firmware. USB only; no battery/BT/wireless.

Board cannot run QMK/ZMK (MCU too new); `conifer/` is from-scratch firmware. Don't assume those stacks' idioms.

## Repo
| Path | What |
|---|---|
| `conifer/` | Firmware, PlatformIO, Arduino. Main code. |
| `kicad/` | `snowlayer.kicad_sch`, `snowlayer.kicad_pcb` |
| `cad/` | STLs, plates + bases, L/R |
| `jlcpcb/` | gerbers, BOM, pick-and-place, IPC netlist |
| `JOURNAL.md` | build log, newest at bottom |
| `README.md` | user build guide + BOM |

Ignore `kicad/snowlayer-backups/` (KiCad autosave).

## Hardware
- MCU: CH32X035G8U6 (RISC-V, 48MHz, 62KB flash, 20KB RAM). One chip drives both halves; right half has no MCU.
- Interconnect: USB-C carrying raw matrix lines. No split protocol/serial/handshake; right rows wired back to same MCU.
- Host: USB-C full-speed (12Mbit), HID boot protocol.

### Matrix
42 keys, 8×6. Rows driven, cols sensed, diodes switch→row, cathode at row (ROW2COL).

| Signal | Pin | | Signal | Pin |
|---|---|-|---|---|
| COL0 | PA0 | | ROW0 | PB0 |
| COL1 | PA1 | | ROW1 | PB3 |
| COL2 | PA2 | | ROW2 | PB4 |
| COL3 | PA3 | | ROW3 | PB6 |
| COL4 | PA4 | | ROW4 | PB8 |
| COL5 | PA5 | | ROW5 | PB7 |
| | | | ROW6 | PB10 |
| | | | ROW7 | PB9 |

- Rows not in pin order: ROW4=PB8/ROW5=PB7, ROW6=PB10/ROW7=PB9 swapped. Authority = pad→net in `snowlayer.kicad_pcb`. `PB1` not a row (not on G8U6; old table listing it shifted rows 1–3). `main.cpp` tables encode real mapping; don't sort them.
- Right half NOT mirrored: both halves COL0→COL5 left to right; `LAYOUT()` maps right straight through. Column flip added twice → right half types mirrored (`hjkl;'` → `';lkjh`). Don't re-add without typing on a working board.
- Rows 0–3 left, 4–7 right. Rows 3, 7 = thumbs, only cols 3–5; rest `KC_NO`. Exception: right thumb row 7 runs COL5→COL3, so `R30 R31 R32` → cols 5, 4, 3 (verified; else Space/Alt swap).
- All cols on GPIOA, rows on GPIOB: one register write selects row, one read samples 6 cols.

## Firmware (`conifer/`)
```
src/main.cpp     scan, debounce, layers, HID reports, bootloader entry
src/keymap.h     THE LAYOUT; only file to touch for remaps
src/keycodes.h   KC_* -> USB HID usage IDs (US QWERTY)
lib/CH32X035_USBHIDKeyboard/   vendored HID stack (NoNamedCat), locally patched
check_keymap.sh  verifies LAYOUT() reorder
platformio.ini   build + upload
flake.nix        dev shell: pio, wchisp, openocd
```

### Scan
Four passes per loop, per-row bitmasks (bit c = col c):
1. Drive row low, settle, read 6 cols at once, per-key debounce.
2. Collect held momentary layers.
3. Resolve held keys against latched layer, build 6KRO report.
4. Send only if changed; pace to `SCAN_INTERVAL_US`.

Passes 2–4 = `updateReport()`, only on scans where debounce committed a change.
- Keys latch layer on press (`heldLayer`); releasing layer key won't strand held key.
- `S(k)` weak shift: follows most recent key only; `(` rolled into `-` sends `-`.
- Send only on change: `USB_write()` blocks till done; unconditional = scan pinned to 1ms poll.

### Tuning (`main.cpp`, reason in comments)
| Constant | Default | Turn when |
|---|---|---|
| `DEBOUNCE_SCANS` | 5 | worn switches chatter → raise |
| `MATRIX_SETTLE_CYCLES` | 150 | key reads as whole column, or far half phantom-presses → raise |
| `SCAN_INTERVAL_US` | 200 | want faster debounce → lower |

- `MATRIX_SETTLE_CYCLES` = busy-loop iterations (few cycles each); 16 settles must fit in `SCAN_INTERVAL_US`.
- `SCAN_INTERVAL_US` makes debounce fixed time (5 × 200µs ≈ 1ms). Saves no power (busy wait).

## Keymap (`src/keymap.h`)
- `LAYOUT()` takes visual order (left then right, row by row, thumbs last), reorders to matrix. No raw indices.
- `KC_*` from `keycodes.h`. `LAYER(n)` = momentary (QMK `MO(n)`). `KC_TRNS` falls through; `KC_NO` dead.
- New layer: bump `NUM_LAYERS`, add to `enum`, add `LAYOUT()` block.
- Mods on own keys; one keycode per key, no mod-tap (thumbs exist for this).
- After edit: `./check_keymap.sh` (expands macro, asserts matrix; catches mis-ordered `LAYOUT()`).

## Build / flash
```bash
nix develop
pio run
pio run -t upload
./check_keymap.sh
```
~13KB flash (21%), ~1.3KB RAM (7%).

### Bootloader
1. Hold A while plugging in. `bootKeyCheck()` in `setup()` before scan loop (survives broken keymap/hung `loop()`). Normal = `16c0:27e5`, A held = `1a86:55e0`. Everyday path, `upload_protocol = isp`. No running-board combo on purpose (would reboot mid-typing).
2. WCH-LinkE over SWD, `upload_protocol = wch-link`. Only path independent of firmware; use after bad flash. Verified:

   | LinkE | Board |
   |---|---|
   | SWDIO | TP2 (PC18/SWDIO) |
   | SWCLK | TP1 (PC19/SWDCK) |
   | GND | any GND |

   Leave probe 3V3 off; board self-powers via USB-C (both plugged fine). `wlink status` shows chip ID. `wlink erase` → blank flash → boot-ROM fallthrough → `wchisp` over USB works again.

- A path: `SystemReset_StartMode(Start_Mode_BOOT)` then reset. Never jump to `0x1FFFF000` (boot ROM hangs with USB/PLL running).
- Boot key checked by matrix position `[1][1]`, not keycode. Moving A in keymap → update `BOOT_KEY_*` in `main.cpp`; `check_keymap.sh` fails if they drift.
- `bootKeyCheck()`: A's column bit set but NOT all six. Exact-match version failed on partly soldered boards (floating positions). All-six rejected = misread matrix; acting on it caused an earlier boot loop.
- No hardware boot path this rev: PC3 (reset-capable) unconnected, no NRST pad, no BOOT0 jumper. "Short the two top-right pads" is WRONG + DAMAGING: that's `SW41`, `SolderJumper_2_Open` between a matrix node and `+5V`.
- Non-blank flash → recovery only via hold-A or WCH-LinkE. Blank flash falls to boot ROM every plug-in (why `wchisp` works on virgin board). Route NRST to a pad next spin.
- Linux USB flashing needs udev rules for VID `1a86`/`4348` (README).

## Conventions
- `lib/` vendored lib locally patched + trimmed: `Print`/`press()`/ASCII map, examples, README removed; only `Keyboard.begin()` + `Keyboard.sendReport()` over `internal/`. Diff before re-vendoring.
- Two `-Wunused-variable` from core `analog.cpp` (`.platformio/packages/`): upstream, leave. Own code clean under `-Wall -Wextra` (`build_src_flags`); keep it.
- Scan path: direct registers (`GPIOA->BCR`, `GPIOB->INDR`), not `digitalWrite`/`digitalRead` (pin-map lookup per call). No `delayMicroseconds()` (64-bit div, no HW divider).

### GPIO traps
Each killed the whole matrix once. Symptom: enumerates fine, no key reports, hold-A does nothing.
- `GPIO_Init()` doesn't enable port clock (`pinMode()` does). Rows use `GPIO_Init()`, so `setup()` must call `RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE)` first. `561fb96` broke board this way. Any port set up only via vendor driver needs clock by hand.
- `CFGHR` write-only; never RMW; use `GPIO_Init()` (RAM shadow).
- PB10 never via `pinMode()` (also `PIN_SERIAL_TX` in variant).
- Rows push-pull: diodes block back-feed, so open-drain buys nothing; push-pull drives release edge.

Dead matrix after GPIO change → check port clock first, then that `upload_protocol` actually flashed.
