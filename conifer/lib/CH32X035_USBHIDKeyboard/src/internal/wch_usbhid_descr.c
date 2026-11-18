#include "wch_usbhid_usb.h"
#include "wch_usbhid_config.h"
#include "wch_usbhid_internal.h"

__attribute__((aligned(4))) unsigned char wch_usbhid_EP0_buffer[EP0_SIZE + 2];
__attribute__((aligned(4))) unsigned char wch_usbhid_EP1_buffer[EP1_SIZE + 2];

// Device Descriptor
const USB_DEV_DESCR wch_usbhid_DevDescr = {
    .bLength            = sizeof(USB_DEV_DESCR),
    .bDescriptorType    = USB_DESCR_TYP_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00, 
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = EP0_SIZE,
    .idVendor           = WCH_USBHID_VENDOR_ID,
    .idProduct          = WCH_USBHID_PRODUCT_ID,
    .bcdDevice          = WCH_USBHID_DEVICE_VERSION,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1
};

// HID Report Descriptor (Standard Keyboard)
const uint8_t wch_usbhid_ReportDescr[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    
    // Modifier Byte (Keys E0-E7)
    0x05, 0x07,       //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,       //   Usage Minimum (0xE0)
    0x29, 0xE7,       //   Usage Maximum (0xE7)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    // Reserved Byte
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x08,       //   Report Size (8)
    0x81, 0x03,       //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    // LEDs (Num Lock, Caps Lock, etc)
    0x95, 0x05,       //   Report Count (5)
    0x75, 0x01,       //   Report Size (1)
    0x05, 0x08,       //   Usage Page (LEDs)
    0x19, 0x01,       //   Usage Minimum (Num Lock)
    0x29, 0x05,       //   Usage Maximum (Kana)
    0x91, 0x02,       //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    
    // LED Padding
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x03,       //   Report Size (3)
    0x91, 0x03,       //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    
    // Key Arrays (6 keys)
    0x95, 0x06,       //   Report Count (6)
    0x75, 0x08,       //   Report Size (8)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x65,       //   Logical Maximum (101)
    0x05, 0x07,       //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,       //   Usage Minimum (0x00)
    0x29, 0x65,       //   Usage Maximum (0x65)
    0x81, 0x00,       //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    0xC0              // End Collection
};

const uint16_t wch_usbhid_ReportDescrLen = sizeof(wch_usbhid_ReportDescr);

// Configuration Descriptor
__attribute__((aligned(4))) const uint8_t wch_usbhid_CfgDescr[] = {
    // Configuration Descriptor
    0x09, 0x02,                     // bLength, bDescriptorType (Config)
    0x22, 0x00,                     // wTotalLength (34 bytes)
    0x01,                           // bNumInterfaces
    0x01,                           // bConfigurationValue
    0x00,                           // iConfiguration
    0x80,                           // bmAttributes (Bus Powered)
    WCH_USBHID_MAX_POWER_mA / 2,    // MaxPower

    // Interface Descriptor
    0x09, 0x04,                     // bLength, bDescriptorType (Interface)
    0x00,                           // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x01,                           // bNumEndpoints
    0x03,                           // bInterfaceClass (HID)
    0x01,                           // bInterfaceSubClass (Boot)
    0x01,                           // bInterfaceProtocol (Keyboard)
    0x00,                           // iInterface

    // HID Descriptor
    0x09, 0x21,                     // bLength, HID
    0x11, 0x01,                     // bcdHID (1.11)
    0x00,                           // bCountryCode
    0x01,                           // bNumDescriptors
    0x22,                           // bDescriptorType (Report)
    (uint8_t)(sizeof(wch_usbhid_ReportDescr)),      // wDescriptorLength L
    (uint8_t)(sizeof(wch_usbhid_ReportDescr) >> 8), // wDescriptorLength H

    // Endpoint 1 IN Descriptor
    0x07, 0x05,                     // bLength, ENDPOINT
    USB_ENDP_ADDR_EP1_IN,           // bEndpointAddress (0x81)
    USB_ENDP_TYPE_INTER,            // bmAttributes (Interrupt)
    0x40, 0x00,                     // wMaxPacketSize (64 bytes)
    0x01                            // bInterval (1ms - Fastest)
};

const uint16_t wch_usbhid_CfgDescrLen = sizeof(wch_usbhid_CfgDescr);

// Strings
const USB_STR_DESCR wch_usbhid_LangDescr = {
  .bLength         = 4,
  .bDescriptorType = USB_DESCR_TYP_STRING,
  .bString         = { WCH_USBHID_LANGUAGE }
};

const USB_STR_DESCR wch_usbhid_ManufDescr = {
  .bLength         = (uint8_t)(2 + 2 * WCH_USBHID_MANUF_LEN),
  .bDescriptorType = USB_DESCR_TYP_STRING,
  .bString         = u"" WCH_USBHID_MANUF_STR
};

const USB_STR_DESCR wch_usbhid_ProdDescr = {
  .bLength         = (uint8_t)(2 + 2 * WCH_USBHID_PROD_LEN),
  .bDescriptorType = USB_DESCR_TYP_STRING,
  .bString         = u"" WCH_USBHID_PROD_STR
};

// The serial carries the chip's unique ID, so it is the one string filled in
// at runtime; the prefix is already in place.
USB_STR_DESCR wch_usbhid_SerDescr = {
  .bLength         = (uint8_t)(2 + 2 * WCH_USBHID_SERIAL_LEN),
  .bDescriptorType = USB_DESCR_TYP_STRING,
  .bString         = u"" WCH_USBHID_SERIAL_PREFIX
};

static void uint32_to_hex(uint32_t value, uint16_t* output) {
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        output[7-i] = hex_chars[(value >> (i * 4)) & 0xF];
    }
}

void generate_serial_descriptor(void) {
    uint16_t* uid = &wch_usbhid_SerDescr.bString[WCH_USBHID_SERIAL_PREFIX_LEN];
    uint32_to_hex(*(volatile uint32_t*)CH32X035_ESIG_UNIID3, &uid[0]);
    uint32_to_hex(*(volatile uint32_t*)CH32X035_ESIG_UNIID2, &uid[8]);
    uint32_to_hex(*(volatile uint32_t*)CH32X035_ESIG_UNIID1, &uid[16]);
}