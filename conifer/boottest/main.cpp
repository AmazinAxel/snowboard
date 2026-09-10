// Matrix test — NOT the keyboard firmware.
//
// Root cause found: CFGHR is write-only on this part. Reading it does not
// return the current configuration, so the earlier
//
//     GPIOB->CFGHR = (GPIOB->CFGHR & ~(0xFu << 8)) | (0x8u << 8);
//
// wrote garbage into the config of every GPIOB pin 8-15 — PB8 (ROW4) and PB9
// (ROW7) among them — which is why no key on any row ever registered. WCH's own
// driver keeps a RAM shadow (CFGHR_tmpB) precisely because the register cannot
// be read back.
//
// This build uses plain pinMode() for every row, PB10 included. pinMode() calls
// GPIO_Init(), which maintains that shadow correctly. PB10 was blamed for the
// USB failure earlier, but that was the COL3 solder bridge, found later.
//
// Reports sticky per-row column masks: byte r, bit c set = that key was seen
// pressed at least once. Press a key, then dump matrixState at leisure.
//
// Flash over SWD:  pio run -e boottest -t upload

#include <USBHIDKeyboard.h>

static const uint8_t COL_PINS[6] = { PA0, PA1, PA2, PA3, PA4, PA5 };
static const uint8_t ROW_PINS[8] = { PB0, PB1, PB3, PB4, PB8, PB6, PB10, PB9 };

// Sticky: bit c set = column c seen pressed on that row. Never cleared.
volatile uint8_t matrixState[8];
volatile uint32_t scanCount;

void setup() {
  Keyboard.begin();

  for (uint8_t c = 0; c < 6; c++) {
    pinMode(COL_PINS[c], OUTPUT);
    digitalWrite(COL_PINS[c], HIGH);
  }
  for (uint8_t r = 0; r < 8; r++) {
    pinMode(ROW_PINS[r], INPUT_PULLUP);
  }
}

void loop() {
  // COL0 is held low permanently, not strobed. A scan drives each column low
  // for only ~50us per pass, so a multimeter averages that to near VDD and a
  // real press shows no visible dip. Holding it low makes the row voltage
  // directly measurable: idle ~VDD, pressed ~0.7V (the diode drop).
  //
  // SW1 is COL0 + D1, so with this build pressing SW1 must pull ROW0 down.
  digitalWrite(COL_PINS[0], LOW);

  for (uint8_t r = 0; r < 8; r++) {
    if (digitalRead(ROW_PINS[r]) == LOW) matrixState[r] |= (1u << 0);
  }
  scanCount++;
}
