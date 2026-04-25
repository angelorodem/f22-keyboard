#include <stddef.h>
#include <stdint.h>

uint8_t *usb_update_sn_string_descriptor(void)
{
    /* Keep CONFIG_USB_DEVICE_SN and avoid RP Pico flash unique-ID reads while USB starts. */
    return NULL;
}