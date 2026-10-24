#include "USBHIDKeyboard.h"

USBHIDKeyboard_ Keyboard;

void USBHIDKeyboard_::begin(void) {
  USB_init();
}

void USBHIDKeyboard_::sendReport(KeyReport* keys) {
  USB_write((uint8_t*)keys, sizeof(KeyReport));
}
