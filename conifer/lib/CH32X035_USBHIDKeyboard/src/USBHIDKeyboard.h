#pragma once

#include <Arduino.h>
#include "internal/wch_usbhid_internal.h"

// HID boot-protocol keyboard report
typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;

class USBHIDKeyboard_ {
public:
  void begin(void);

  // Blocks until the host has taken the report
  void sendReport(KeyReport* keys);
};

extern USBHIDKeyboard_ Keyboard;
