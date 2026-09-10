// Snowlayer — 42-key split keyboard firmware for the CH32X035G8U6.
//
// Single MCU drives both halves over the USB-C interconnect: 6 columns
// (PA0-PA5) x 8 rows (PB0-PB10). Rows 0-3 are the left half, rows 4-7 the
// right. See CLAUDE.md for the wiring table.
//
// To change your layout, edit keymap.h. You should not need to touch this file.

#include <Arduino.h>
#include <USBHIDKeyboard.h>

#include "keycodes.h"
#include "keymap.h"

// --- Wiring -----------------------------------------------------------------
// Rows are driven (outputs), columns are sensed (inputs with pull-up).
// This is ROW2COL in QMK terms, and it is set by the diode orientation on the
// PCB: the cathode sits at the row (D1 pin 1 = ROW0, pin 2 = anode = the switch
// side). Current can therefore only flow switch -> row, so the row is the end
// that must be pulled LOW and the column is the end that is sensed.
//
// Every column is on GPIOA and every row on GPIOB, which lets the scan touch
// the registers directly: one write to select a row, one read to sample all
// six columns at once. Arduino's digitalRead/digitalWrite would do a pin-map
// lookup per pin — 48 of them per full scan.
static const uint8_t COL_PINS[6] = { PA0, PA1, PA2, PA3, PA4, PA5 };
static const uint8_t ROW_PINS[8] = { PB0, PB3, PB4, PB6, PB8, PB7, PB10, PB9 };
//                        ROW index:  0    1    2    3    4    5    6     7
// Taken from the MCU pad -> net mapping in snowlayer.kicad_pcb, not from pin
// order: ROW0=pad13, ROW1=pad14, ROW2=pad15, ROW3=pad17, ROW4=pad19,
// ROW5=pad18, ROW6=pad21, ROW7=pad20. Two traps in there — ROW4/ROW5 are
// swapped relative to pad order (ROW5=PB7 on pad 18, ROW4=PB8 on pad 19), and
// PB1 is not a row at all: it does not exist on the G8U6 package.

// GPIO bit positions, in the same order as the tables above.
static const uint8_t COL_BITS[6] = { 0, 1, 2, 3, 4, 5 };          // PA0..PA5
static const uint8_t ROW_BITS[8] = { 0, 3, 4, 6, 8, 7, 10, 9 };   // PB pins

// Mask of every column bit, so one INDR read covers the whole row.
// Columns are PA0..PA5, so this is simply bits 0-5.
#define COL_MASK 0x3Fu

#define NUM_ROWS 8
#define NUM_COLS 6
#define COL_ALL  0x3F  // bits 0..5, one per column

// --- Tuning -----------------------------------------------------------------
// Debounce: a key must read the same for this many consecutive scans before
// the change is reported. A scan is ~100us, so 5 scans is ~0.5ms — well under
// the 1ms USB polling interval, so debouncing costs no perceptible latency.
// Raise this if you get chatter from worn switches.
// ponytail: per-key counters, plenty for 42 keys; no fancier algorithm needed.
static const uint8_t DEBOUNCE_SCANS = 5;

// Settle time for a row line after it is driven, in CPU cycles at 48MHz
// (~21ns each). The interconnect between halves is the longest run, so this
// is the knob to turn if the far half ever reports phantom presses.
// 150 cycles is ~3us, well past the RC of the trace plus the diode.
// Arduino's delayMicroseconds() does 64-bit division on a core with no
// hardware divider, which costs more than the delay itself at this scale.
static const uint32_t MATRIX_SETTLE_CYCLES = 150;

static inline void settleDelay() {
  for (uint32_t i = 0; i < MATRIX_SETTLE_CYCLES; i++) __asm__ volatile("nop");
}

// Target time for one full matrix scan, in microseconds. 200us gives five
// scans per 1ms USB frame — enough for the debounce filter to settle between
// polls without spinning the core flat out. Lower it for a faster debounce
// response, raise it to trade latency for current draw.
static const uint32_t SCAN_INTERVAL_US = 200;

// Both of these keys held together reboots into the USB bootloader.
// Physical positions, not keycodes, so they work on any layer.
#define BOOT_KEY_A_ROW 0  // Q  (ROW0, COL1)
#define BOOT_KEY_A_COL 1
#define BOOT_KEY_B_ROW 1  // A  (ROW1, COL1)
#define BOOT_KEY_B_COL 1

// --- State ------------------------------------------------------------------
// Debounced matrix state, one bit per column (bit c = column c) per row. A set
// bit means pressed. Bitmasks instead of a bool[8][6] so "is anything down?"
// is a single OR and the report loop can skip empty rows outright.
static uint8_t keyDown[NUM_ROWS];
static uint8_t lastRead[NUM_ROWS];               // last raw sample
static uint8_t stableCount[NUM_ROWS][NUM_COLS];
static uint8_t heldLayer[NUM_ROWS][NUM_COLS];    // layer a key resolved on

#define KEY_IS_DOWN(r, c) ((keyDown[(r)] >> (c)) & 1u)

static KeyReport report;
static KeyReport lastReport;

// --- One-shot modifiers -----------------------------------------------------
// A one-shot ("sticky") modifier is tapped rather than held: tap Shift, tap a
// letter, get a capital. This is what lets the modifiers live on the thumbs
// instead of the pinkies without asking the hand to hold anything down.
//
// Two bitmasks, both in KC_LCTL..KC_RGUI bit order:
//   oneshotMods   armed and waiting for a key to apply to
//   oneshotUsed   armed modifiers that a key has already consumed; they are
//                 cleared once that key is released, not immediately, so the
//                 host sees the modifier held for the whole of the keypress.
//
// Deliberately no timeout. A one-shot stays armed until it is used or tapped
// again, which means there is no window to beat and nothing behaves
// differently when typing fast — the failure mode that rules out mod-tap and
// tap dance on this board.
static uint8_t oneshotMods;
static uint8_t oneshotUsed;

// Held-key state for the one-shot edge detection: bit c per row, tracking
// which keys were down on the previous scan.
static uint8_t prevKeyDown[NUM_ROWS];

// Timestamp of the last scan, for pacing. Unsigned subtraction makes the
// comparison correct across the micros() rollover.
static uint32_t lastScanUs;

// Reboot into the CH32X035 factory USB bootloader so the board can be
// re-flashed with wchisp over USB-C, no button shorting required.
//
// Do NOT jump to 0x1FFFF000 directly: the bootloader expects the chip in its
// reset state, and with USB and the PLL already running it hangs. The supported
// path is to set the BOOT start-mode bit in FLASH->STATR and then reset, so the
// boot ROM runs from a clean chip. The bit is sticky only across this reset.
static void jumpToBootloader() {
  Keyboard.releaseAll();
  Keyboard.end();
  delay(50);  // let the host process the key release before we vanish

  __disable_irq();
  SystemReset_StartMode(Start_Mode_BOOT);
  NVIC_SystemReset();
  while (1) {}  // not reached
}

// Power-on recovery check: is A held right now?
//
// Reads the matrix directly rather than going through the debounced state,
// because this runs before the scan loop exists.
//
// Only A, not the Q+A the running board uses. This is the last way back in if
// a flash goes bad, so it is worth making as easy to hit as possible — one key
// held through a replug is far more reliable than two, especially with jumper
// wires on a bare PCB. The cost is that a stuck or shorted A at power-on drops
// you to the bootloader instead of the keyboard, which a replug undoes.
static void bootCombo() {
  // Let the column pull-ups charge the lines before the first sample. The rows
  // were configured microseconds ago and an undriven column still reads low
  // until its pull-up wins, which would look like every key held at once.
  for (uint32_t i = 0; i < 20000; i++) __asm__ volatile("nop");

  // Sample twice with the row released in between. A real held key reads
  // pressed both times; a line that has not settled does not.
  GPIOB->BCR = (1u << ROW_BITS[BOOT_KEY_B_ROW]);  // select A's row
  settleDelay();
  uint32_t first = ~GPIOA->INDR & COL_MASK;
  GPIOB->BSHR = (1u << ROW_BITS[BOOT_KEY_B_ROW]);  // release it

  settleDelay();

  GPIOB->BCR = (1u << ROW_BITS[BOOT_KEY_B_ROW]);
  settleDelay();
  uint32_t second = ~GPIOA->INDR & COL_MASK;
  GPIOB->BSHR = (1u << ROW_BITS[BOOT_KEY_B_ROW]);

  uint32_t sampled = first & second;

  // A must read pressed. Deliberately not an exact match on A alone: the
  // neighbouring switches on this row may not be soldered yet, and an unpopulated
  // position can float. Requiring exactly one bit made the hatch impossible to
  // trigger on a partially built board.
  if (!(sampled & (1u << COL_BITS[BOOT_KEY_B_COL]))) return;

  // Still refuse the degenerate case: every column set means the matrix is
  // misreading rather than the user holding the whole row. That reading is what
  // caused the boot loop this check was disabled for.
  if (sampled == COL_MASK) return;

  // USB has been initialised by the time this runs, but nothing is enumerated
  // yet and the host has seen no reports, so there is nothing to tear down —
  // set the start-mode flag and reset.
  __disable_irq();
  SystemReset_StartMode(Start_Mode_BOOT);
  NVIC_SystemReset();
}

// Resolve a key position against the layer stack: search from the highest
// active layer down, skipping KC_TRNS, so transparent keys fall through.
static uint8_t resolveKey(uint8_t layerMask, uint8_t r, uint8_t c) {
  for (int8_t l = NUM_LAYERS - 1; l >= 0; l--) {
    if (l != BASE && !(layerMask & (1 << l))) continue;
    uint8_t k = KEYMAP[l][r][c];
    if (k != KC_TRNS) return k;
  }
  return KC_NO;
}

void setup() {
  // USB first, before any pinMode(). The matrix setup below reconfigures GPIO
  // and AFIO, and doing that ahead of USB_init() stops the device enumerating.
  Keyboard.begin();

  for (uint8_t c = 0; c < NUM_COLS; c++) {
    pinMode(COL_PINS[c], INPUT_PULLUP);
  }
  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    // PB10 must not go through pinMode(). The variant header also maps it to
    // PIN_SERIAL_TX, and configuring it via the Arduino pin map clobbers the
    // USB pin setup — the device then never enumerates at all. Bisected: every
    // other row is fine, adding PB10 alone kills it. Set it up by hand instead.
    if (ROW_PINS[r] == PB10) continue;
    pinMode(ROW_PINS[r], OUTPUT);
    digitalWrite(ROW_PINS[r], HIGH);  // idle high = not selected
  }

  // PB10 (ROW6), push-pull output, via the vendor driver rather than a
  // read-modify-write of CFGHR.
  //
  // CFGHR is write-only on this part: reading it does not return the current
  // configuration, which is why ch32x035_gpio.c keeps a RAM shadow (CFGHR_tmpB)
  // and writes the whole register from that. A `GPIOB->CFGHR = (GPIOB->CFGHR &
  // ~mask) | bits` here silently reconfigures every other pin 8-15 — including
  // PB8 (ROW4) and PB9 (ROW7) — and the matrix goes dead.
  GPIO_InitTypeDef pb10;
  pb10.GPIO_Pin   = GPIO_Pin_10;
  pb10.GPIO_Speed = GPIO_Speed_50MHz;
  pb10.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &pb10);
  GPIOB->BSHR = (1u << 10);  // idle high, same as the rows above

  // All matrix/report state is static, so it is already zero-initialised.

  // Recovery hatch. Hold the boot combo while plugging the board in and we go
  // straight to the bootloader, before USB or the scan loop start. That makes
  // this the one path that still works if the keymap is wrong or loop() hangs:
  // the only things it depends on are the GPIO setup above and a reset.
  //
  // Read raw, with no debounce — the key has been held since power-on, so the
  // line is long settled, and a stray sample here only costs a replug.
  //
  // Was disabled for a while because it fired on every boot. The cause was the
  // scan direction: the old code drove a column and read rows, which against
  // this board's ROW2COL diodes reads as every key held. With the direction
  // fixed and the exact-match guard in bootCombo(), it is safe to re-enable.
  bootCombo();
}

#ifdef DEBUG_ROW
// Diagnostic: type a row's raw column bitmask as hex whenever it changes.
//
// Build with -DDEBUG_ROW=5 to trace row 5. Open a text editor, press a key on
// that row, and read what appears: a bit that never appears means the MCU never
// saw the press, which puts the fault at the switch, diode, or trace rather
// than anywhere in this file.
//
// Sends its own reports directly and does not touch keyDown/report, so normal
// typing still works alongside it. Blocking (each USB_write waits for the
// transfer), which is fine for a debug build and is why it is compiled out by
// default.
static void debugType(uint8_t usage) {
  KeyReport k;
  memset(&k, 0, sizeof(k));
  k.keys[0] = usage;
  Keyboard.sendReport(&k);
  memset(&k, 0, sizeof(k));
  Keyboard.sendReport(&k);  // release, so the host sees a discrete tap
}

static void debugRow(uint8_t r, uint32_t sampled) {
  static uint32_t lastDebug = 0xFFFFFFFFu;
  if (sampled == lastDebug) return;
  lastDebug = sampled;

  // Row number, then the mask as two hex nibbles, then a space.
  static const uint8_t hex[16] = {
    KC_0, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7,
    KC_8, KC_9, KC_A, KC_B, KC_C, KC_D, KC_E, KC_F
  };
  debugType(hex[r & 0xF]);
  debugType(KC_MINS);
  debugType(hex[(sampled >> 4) & 0xF]);
  debugType(hex[sampled & 0xF]);
  debugType(KC_SPC);
}
#endif

void loop() {
  // Pass 1: scan the whole matrix and update the debounced state.
  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    GPIOB->BCR = (1u << ROW_BITS[r]);   // drive this row LOW (select)
    settleDelay();

    // Sample all six columns in one read. A pressed key pulls its column LOW,
    // so invert: a 1 in `sampled` means pressed.
    uint32_t sampled = ~GPIOA->INDR & COL_MASK;
    GPIOB->BSHR = (1u << ROW_BITS[r]);  // release the row (deselect)

#ifdef DEBUG_ROW
    // Type the raw column bits for one row whenever they change, so a press
    // that never reaches the host can still be seen at the pin. Prints e.g.
    // "5:02 " for column 1 down on row 5. Undebounced and deliberately before
    // any keymap lookup: this reports what the MCU read, nothing further.
    if (r == DEBUG_ROW) debugRow(r, sampled);
#endif

    for (uint8_t c = 0; c < NUM_COLS; c++) {
      uint8_t pressed = (sampled >> COL_BITS[c]) & 1u;
      uint8_t bit = (1u << c);

      if (pressed == ((lastRead[r] >> c) & 1u)) {
        if (stableCount[r][c] < DEBOUNCE_SCANS) stableCount[r][c]++;
      } else {
        stableCount[r][c] = 0;
        lastRead[r] ^= bit;
      }

      if (stableCount[r][c] >= DEBOUNCE_SCANS) {
        if (pressed) keyDown[r] |= bit;
        else         keyDown[r] &= ~bit;
      }
    }
  }

  // Escape hatch: Q + A together drops to the bootloader.
  //
  // DISABLED alongside the power-on check: if the matrix ever reads all-down,
  // this fires on the first scan and resets the board before the host finishes
  // enumerating. Recovery is the WCH-LinkE on TP1/TP2 until the scan is trusted.
  // if (KEY_IS_DOWN(BOOT_KEY_A_ROW, BOOT_KEY_A_COL) &&
  //     KEY_IS_DOWN(BOOT_KEY_B_ROW, BOOT_KEY_B_COL)) {
  //   jumpToBootloader();
  // }
  (void)jumpToBootloader;

  // Pass 2: collect held layers. Layer keys are evaluated before anything else
  // so that pressing a layer key and a key under it in the same scan resolves
  // on the new layer.
  uint8_t layerMask = 0;
  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    uint8_t down = keyDown[r];
    while (down) {
      uint8_t c = __builtin_ctz(down);
      down &= down - 1;  // clear lowest set bit
      uint8_t k = KEYMAP[BASE][r][c];
      if (IS_LAYER(k)) layerMask |= (1 << LAYER_NUM(k));
    }
  }

  // Pass 3: build the HID report.
  // A key keeps the layer it was pressed on (heldLayer) so releasing a layer
  // key mid-press doesn't strand a key down on the host.
  memset(&report, 0, sizeof(report));
  uint8_t slot = 0;

  // Set while walking the matrix if any non-modifier key is down, so the armed
  // one-shots can be released at the end of the pass once nothing is consuming
  // them any more.
  bool anyNormalKeyDown = false;

  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    uint8_t down = keyDown[r];

    // Drop the latch on every key in this row that is no longer held.
    uint8_t released = ~down & COL_ALL;
    while (released) {
      uint8_t c = __builtin_ctz(released);
      released &= released - 1;
      heldLayer[r][c] = 0;
    }

    // Keys that went down since the last scan. One-shots act on the press
    // edge: arming and consuming must each happen exactly once per press, not
    // once per scan.
    uint8_t pressedEdge = down & ~prevKeyDown[r];

    while (down) {
      uint8_t c = __builtin_ctz(down);
      down &= down - 1;
      bool isEdge = (pressedEdge >> c) & 1u;

      uint8_t base = KEYMAP[BASE][r][c];
      if (IS_LAYER(base)) continue;  // layer keys emit nothing themselves

      // Latch the layer on the press edge, reuse it until released.
      if (!heldLayer[r][c]) heldLayer[r][c] = layerMask | 0x80;  // 0x80 = "set"
      uint8_t k = resolveKey(heldLayer[r][c] & 0x7F, r, c);

      if (k == KC_NO) continue;
      if (IS_MOD(k)) {
        report.modifiers |= MOD_BIT(k);
      } else if (IS_OSM(k)) {
        // Holding a one-shot works as a plain modifier — games bind held Shift
        // to sprint and held Ctrl to crouch, and those have to keep working.
        // The armed state below is what makes the *tap* case do something
        // different, and it costs nothing while the key is still down.
        report.modifiers |= OSM_BIT(k);
        // Toggle on the press edge, so tapping an armed one-shot disarms it
        // rather than re-arming it forever.
        if (isEdge) oneshotMods ^= OSM_BIT(k);
      } else if (slot < 6) {
        report.keys[slot++] = k;
        anyNormalKeyDown = true;
        // This key consumes whatever is armed. Marking rather than clearing:
        // the modifier has to stay in the report for as long as the key it
        // applies to is held, or the host sees Shift release mid-keypress.
        if (isEdge) oneshotUsed |= oneshotMods;
      } else {
        anyNormalKeyDown = true;
      }
      // Beyond 6 keys the extras are dropped — standard boot-protocol NKRO
      // limit. ponytail: fine for typing; needs a second endpoint to exceed.
    }

    prevKeyDown[r] = keyDown[r];
  }

  // Apply the armed one-shots to this report, then retire the consumed ones
  // once the key that used them is up. Ordering matters: the modifier bits go
  // in before the clear, so the final report of a tapped-Shift-then-letter
  // sequence still carries Shift alongside the letter.
  report.modifiers |= oneshotMods;
  if (!anyNormalKeyDown) {
    oneshotMods &= ~oneshotUsed;
    oneshotUsed = 0;
  }

  // Pass 4: send only when something actually changed. USB_write blocks until
  // the transfer completes, so an unconditional send would cap the scan rate
  // at the 1ms polling interval for no benefit.
  if (memcmp(&report, &lastReport, sizeof(report)) != 0) {
    Keyboard.sendReport(&report);
    lastReport = report;
  }

  // Pace the scan. The host only collects a report every 1ms (bInterval=1), so
  // scanning flat out just burns current re-reading pins nobody will ask about.
  // At SCAN_INTERVAL_US the debounce window (DEBOUNCE_SCANS scans) still closes
  // inside a single USB frame, so this costs no perceptible latency.
  // ponytail: a plain wait; WFI + a timer IRQ would idle the core instead, add
  // that if measured idle current actually matters.
  while (micros() - lastScanUs < SCAN_INTERVAL_US) { }
  lastScanUs = micros();
}
