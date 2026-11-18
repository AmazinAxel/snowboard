#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef WCH_USBHID_USB_DEFS
#define WCH_USBHID_USB_DEFS

#define USB_DESCR_TYP_DEVICE    0x01
#define USB_DESCR_TYP_CONFIG    0x02
#define USB_DESCR_TYP_STRING    0x03
#define USB_DESCR_TYP_HID       0x21
#define USB_DESCR_TYP_REPORT    0x22

#define USB_ENDP_TYPE_INTER     0x03
#define USB_ENDP_ADDR_EP1_IN    0x81

// HID Requests
#define HID_REQ_GET_IDLE        0x02
#define HID_REQ_GET_PROTO       0x03
#define HID_REQ_SET_REPORT      0x09
#define HID_REQ_SET_IDLE        0x0A
#define HID_REQ_SET_PROTO       0x0B

typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} USB_DEV_DESCR, *PUSB_DEV_DESCR;

/* aligned(2) is the struct's real alignment: bString sits at offset 2 after
   two uint8_t fields, so taking a uint16_t* to it is safe. Stating it lets
   GCC drop the -Waddress-of-packed-member false positive; layout is unchanged. */
typedef struct __attribute__((packed, aligned(2))) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bString[64];
} USB_STR_DESCR, *PUSB_STR_DESCR;

#endif // WCH_USBHID_USB_DEFS

#ifdef __cplusplus
}
#endif