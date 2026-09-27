# Snowlayer

A 42-key split ergonomic keyboard: custom PCB, 3D-printed case, and its own
firmware. USB only — no battery, no Bluetooth, no wireless.

The defining constraint: **this board cannot run QMK or ZMK.** The MCU is too
new for either to support, so `conifer/` is a from-scratch firmware. Don't
reach for upstream keyboard-firmware idioms that assume those stacks exist.

## Layout of this repo

| Path        | What it is                                                        |
| ----------- | ----------------------------------------------------------------- |
| `conifer/`  | The firmware. PlatformIO project, Arduino framework. **Main code.** |
| `kicad/`    | Schematic + PCB (`snowlayer.kicad_sch`, `snowlayer.kicad_pcb`).    |
| `cad/`      | STLs for the printed plates and bases, left and right.             |
| `jlcpcb/`   | Fab outputs: gerbers, BOM, pick-and-place, IPC netlist.            |
| `JOURNAL.md`| Build log, newest entries at the bottom.                           |
| `README.md` | User-facing build guide and bill of materials.                     |

`kicad/snowlayer-backups/` is KiCad's own autosave churn — ignore it.

## Hardware

- **MCU:** CH32X035G8U6 (RISC-V, 48MHz, 62KB flash, 20KB RAM). One chip drives
  *both* halves; the right half has no MCU of its own.
- **Interconnect:** USB-C between halves, carrying the raw matrix lines. This is
  not a data link — no split protocol, no serial, no handshake. The right half's
  rows are simply wired back to the same MCU.
- **Host link:** USB-C, full-speed (12Mbit), HID boot protocol.

### Matrix

42 keys on an 8×6 matrix. Rows are driven, columns are sensed, diodes run
switch→row with the cathode at the row (ROW2COL).

| Signal | Pin  | | Signal | Pin  |
| ------ | ---- |-| ------ | ---- |
| COL0   | PA0  | | ROW0   | PB0  |
| COL1   | PA1  | | ROW1   | PB3  |
| COL2   | PA2  | | ROW2   | PB4  |
| COL3   | PA3  | | ROW3   | PB6  |
| COL4   | PA4  | | ROW4   | PB8  |
| COL5   | PA5  | | ROW5   | PB7  |
|        |      | | ROW6   | PB10 |
|        |      | | ROW7   | PB9  |

Two things here bite if you forget them:

1. **Rows are not in pin order.** ROW4=PB8 but ROW5=PB7, and ROW6=PB10 but
   ROW7=PB9 — both pairs swapped. The authority is the MCU pad → net mapping in
   `snowlayer.kicad_pcb`, not the pin numbering. `PB1` is *not* a row: it does
   not exist on the G8U6 package, and an earlier revision of this table listing
   it as ROW1 is what shifted every row 1–3 by one pin. The tables in `main.cpp`
   encode the real mapping; don't "tidy" them into ascending order.
2. **The right half is *not* mirrored.** Both halves run COL0→COL5 left to
   right as you look at the board, so `LAYOUT()` maps the right half straight
   through. A column flip has been added twice, and on hardware it makes the
   right half type mirrored (`hjkl;'` → `';lkjh`). Don't add it back unless
   you've typed on a working board to check.

Rows 0–3 are the left half, rows 4–7 the right. Rows 3 and 7 are the thumb
clusters and only populate columns 3–5; the other three positions are `KC_NO`.

All columns live on GPIOA and all rows on GPIOB, which is what lets the scan
select a row with one register write and sample all six columns with one read.

## Firmware (`conifer/`)

```
src/main.cpp     Matrix scan, debounce, layer resolution, HID reports, bootloader entry
src/keymap.h     THE LAYOUT. Edit this to remap keys. Nothing else should need touching.
src/keycodes.h   KC_* names -> raw USB HID usage IDs (US QWERTY)
lib/CH32X035_USBHIDKeyboard/   Vendored USB HID stack (NoNamedCat), locally patched
check_keymap.sh  Verifies the keymap macro reorders correctly
platformio.ini   Build + upload config
flake.nix        Nix dev shell with pio, wchisp, openocd
```

### How a scan works

Four passes per loop, all state held as per-row bitmasks (bit *c* = column *c*):

1. Drive each row low, settle, read all 6 columns at once, run per-key debounce.
2. Collect which momentary layers are held.
3. Resolve each held key against its latched layer, build the 6KRO HID report.
4. Send only if the report changed, then pace the loop to `SCAN_INTERVAL_US`.

Passes 2–4 live in `updateReport()`, which runs only on scans where the
debounce committed a change. Most scans are just the matrix read and the
pacing wait.

Two details worth knowing before changing this:

- **Keys latch their layer on press** (`heldLayer`). Releasing a layer key while
  a key under it is still down won't strand that key on the host.
- **Reports are sent only on change.** `USB_write()` blocks until the transfer
  completes, so sending unconditionally would pin the scan rate to the 1ms poll.

### Tuning knobs

All in `main.cpp`, all with the physical reason to change them in a comment:

| Constant             | Default | Turn it when                              |
| -------------------- | ------- | ----------------------------------------- |
| `DEBOUNCE_SCANS`     | 5       | Worn switches chatter → raise             |
| `MATRIX_SETTLE_CYCLES` | 150   | One key reads as its whole column, or far half phantom-presses → raise |
| `SCAN_INTERVAL_US`   | 200     | Want faster debounce → lower |

`MATRIX_SETTLE_CYCLES` counts busy-loop iterations, not CPU cycles; each
iteration takes a few cycles. 16 settles per scan have to fit inside
`SCAN_INTERVAL_US`.

`SCAN_INTERVAL_US` exists to make `DEBOUNCE_SCANS` a fixed amount of time:
5 scans × 200µs ≈ 1ms. It saves no power, because the wait is a busy loop.

## Editing the keymap

Open `src/keymap.h`. The `LAYOUT()` macro takes keys in visual order — left
half then right half, row by row, thumbs last — and reorders them into the
electrical matrix. You never write raw row/column indices.

- Keycodes are `KC_*` from `keycodes.h`.
- `LAYER(n)` on a key = momentary layer *n* while held (QMK's `MO(n)`).
- `KC_TRNS` falls through to the layer below; `KC_NO` is a dead key.
- Add a layer: bump `NUM_LAYERS`, add to the `enum`, add a `LAYOUT()` block.

Modifiers go on their own keys — one keycode per key, no mod-tap. The thumb
clusters exist for exactly this.

After editing, run `./check_keymap.sh`. It expands the macro and asserts the
resulting matrix, so it catches a mis-ordered `LAYOUT()` — the failure mode
that would otherwise only show up as scrambled keys on real hardware.

## Build and flash

```bash
nix develop            # or ensure pio is on PATH
pio run                # build
pio run -t upload      # flash
./check_keymap.sh      # verify the layout after editing keymap.h
```

Current footprint: ~13KB flash (21% of 62KB), ~1.3KB RAM (7% of 20KB).

### Getting into the bootloader

1. **Hold A while plugging the board in.** Checked in `setup()` by
   `bootKeyCheck()` before the scan loop starts, so it survives a broken keymap
   or a hung `loop()`. Verified working: a normal plug-in enumerates
   `16c0:27e5`, holding A gives `1a86:55e0`. This is the everyday path —
   `upload_protocol = isp`. There is deliberately no key combo on a running
   board; it would reboot mid-typing whenever the combo came up.
2. **WCH-LinkE** on SWD, if USB is unavailable — set
   `upload_protocol = wch-link` in `platformio.ini`. This is the only path that
   does not depend on the firmware working, and the one to reach for after a
   bad flash. Verified working on this board:

   | LinkE | Board                |
   | ----- | -------------------- |
   | SWDIO | TP2 (PC18/SWDIO)     |
   | SWCLK | TP1 (PC19/SWDCK)     |
   | GND   | any GND              |

   Leave the probe's 3V3 disconnected; the board self-powers from its USB-C,
   and both plugged in at once is fine. `wlink status` should report the chip
   ID. `wlink erase` returns flash to blank, which restores the boot-ROM
   fallthrough and makes `wchisp` work over USB again.

The A path calls `SystemReset_StartMode(Start_Mode_BOOT)` and then resets.
Do *not* jump to `0x1FFFF000` directly — the boot ROM expects a chip in its
reset state and hangs if USB and the PLL are already running.

The boot key is checked against its *physical matrix position*, not a keycode,
so it works whatever the keymap says. A is `[1][1]`; if you ever move it in the
keymap, update `BOOT_KEY_*` in `main.cpp` to match — `check_keymap.sh` will
fail loudly if they drift apart.

`bootKeyCheck()` requires A's column bit set but *not* all six columns. The guard
was originally an exact match on A alone, which made the hatch impossible to
trigger on a partially soldered board: an unpopulated switch position can float,
and any stray bit on that row failed the comparison. The all-columns-set case is
still rejected — that reading means the matrix is misreading rather than the
whole row being held, and acting on it is what caused an earlier boot loop.

**There is no hardware boot path on this revision.** PC3 (the reset-capable
pin) is left unconnected, so there is no NRST pad to pull low, and the board
carries no BOOT0 jumper. Earlier revisions of this file described "shorting the
two pads on the top right" — that is **wrong and damaging**. Those pads are
`SW41`, a `SolderJumper_2_Open` wired between a matrix key node and `+5V`;
shorting it drives 5V into a GPIO/diode node. It has never been a boot path.

Consequence: once flash is non-blank, recovery depends on the firmware's hold-A
path or a WCH-LinkE. While flash *is* blank the chip falls through to the boot
ROM on every plug-in, which is why `wchisp` works out of the box on a virgin
board. Worth routing NRST to a pad on the next PCB spin.

On Linux, flashing over USB needs udev rules for VID `1a86`/`4348` — see the
README.

## Conventions

- The vendored library under `lib/` is **locally patched**. Don't blindly
  re-vendor it from upstream; diff first.
- The Arduino core in `.platformio/packages/` emits two `-Wunused-variable`
  warnings from `analog.cpp`. Those are upstream and regenerate on reinstall —
  leave them. Our own code builds clean under `-Wall -Wextra`, enforced via
  `build_src_flags`, and should stay that way.
- Prefer direct register access (`GPIOA->BCR`, `GPIOB->INDR`) over
  `digitalWrite`/`digitalRead` in the scan path; the Arduino wrappers do a
  pin-map lookup per call. Likewise avoid `delayMicroseconds()` there — it does
  64-bit division on a core with no hardware divider.

### GPIO setup traps

Every one of these has killed the whole matrix at least once. The symptom is
always the same: the board enumerates fine, but no key ever sends a report and
holding A at plug-in does nothing.

- **`GPIO_Init()` does not enable the port clock.** `pinMode()` does, as a side
  effect. The rows go through `GPIO_Init()`, so `setup()` must call
  `RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE)` first. Without it,
  every write to GPIOB is dropped and no row ever pulls low. This is how
  `561fb96` broke the board: it moved the last GPIOB pins off `pinMode()`, and
  the clock those calls had been enabling went with them. Any port you set up
  purely through the vendor driver needs its clock enabled by hand.
- **`CFGHR` is write-only.** Never read-modify-write it; go through
  `GPIO_Init()`, which keeps a RAM shadow.
- **PB10 must not go through `pinMode()`** — it is also `PIN_SERIAL_TX` in the
  variant header.
- **Rows are push-pull.** With a diode on every switch a deselected row driven
  high can't back-feed a column, so open-drain buys nothing, and push-pull
  drives the release edge instead of waiting on the pull-up RC.

If the matrix goes dead after a GPIO change, check the port clock first, and
make sure `upload_protocol` really flashed (see `platformio.ini`).
