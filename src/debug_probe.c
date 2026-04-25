#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#if defined(CONFIG_F22_DEBUG_PROBE_USB_STATE)
#include <zephyr/drivers/usb/usb_dc.h>
#include <zephyr/usb/usb_device.h>
#endif

#define DEBUG_LED_NODE DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(DEBUG_LED_NODE, okay)
#error "F22 debug probe requires a led0 alias"
#endif

static const struct gpio_dt_spec debug_led = GPIO_DT_SPEC_GET(DEBUG_LED_NODE, gpios);

static int configure_debug_led(void)
{
    if (!gpio_is_ready_dt(&debug_led)) {
        return -ENODEV;
    }

    return gpio_pin_configure_dt(&debug_led, GPIO_OUTPUT_INACTIVE);
}

#if defined(CONFIG_F22_DEBUG_PROBE_BLINK)
static void blink_timer_handler(struct k_timer *timer)
{
    static bool led_on;

    ARG_UNUSED(timer);

    led_on = !led_on;
    (void)gpio_pin_set_dt(&debug_led, led_on);
}

K_TIMER_DEFINE(blink_timer, blink_timer_handler, NULL);
#endif

#if defined(CONFIG_F22_DEBUG_PROBE_USB_STATE)
enum usb_probe_pattern {
    USB_PROBE_PATTERN_OFF = 0,
    USB_PROBE_PATTERN_ARMED = 1,
    USB_PROBE_PATTERN_RESET = 2,
    USB_PROBE_PATTERN_CONNECTED = 3,
    USB_PROBE_PATTERN_CONFIGURED = 4,
    USB_PROBE_PATTERN_DISCONNECTED = 5,
    USB_PROBE_PATTERN_SUSPENDED = 6,
};

static atomic_t usb_pattern_count;
static atomic_t usb_pattern_phase;
static atomic_t usb_enable_failed;

static void set_usb_pattern(enum usb_probe_pattern pattern)
{
    atomic_set(&usb_pattern_count, pattern);
    atomic_set(&usb_pattern_phase, 0);
}

static void usb_state_timer_handler(struct k_timer *timer)
{
    int count;
    int phase;
    int cycle_ticks;
    bool led_on;

    ARG_UNUSED(timer);

    if (atomic_get(&usb_enable_failed) != 0) {
        (void)gpio_pin_set_dt(&debug_led, 1);
        return;
    }

    count = atomic_get(&usb_pattern_count);
    if (count <= 0) {
        (void)gpio_pin_set_dt(&debug_led, 0);
        return;
    }

    phase = atomic_get(&usb_pattern_phase);
    cycle_ticks = (count * 2) + 6;
    led_on = (phase < (count * 2)) && ((phase % 2) == 0);

    (void)gpio_pin_set_dt(&debug_led, led_on);
    atomic_set(&usb_pattern_phase, (phase + 1) % cycle_ticks);
}

static void usb_status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
    ARG_UNUSED(param);

    switch (status) {
    case USB_DC_RESET:
        set_usb_pattern(USB_PROBE_PATTERN_RESET);
        break;
    case USB_DC_CONNECTED:
        set_usb_pattern(USB_PROBE_PATTERN_CONNECTED);
        break;
    case USB_DC_CONFIGURED:
        set_usb_pattern(USB_PROBE_PATTERN_CONFIGURED);
        break;
    case USB_DC_DISCONNECTED:
        set_usb_pattern(USB_PROBE_PATTERN_DISCONNECTED);
        break;
    case USB_DC_SUSPEND:
        set_usb_pattern(USB_PROBE_PATTERN_SUSPENDED);
        break;
    case USB_DC_RESUME:
        set_usb_pattern(USB_PROBE_PATTERN_CONNECTED);
        break;
    default:
        break;
    }
}

K_TIMER_DEFINE(usb_state_timer, usb_state_timer_handler, NULL);
#endif

static int f22_debug_probe_init(void)
{
    int ret;

    ret = configure_debug_led();
    if (ret != 0) {
        return ret;
    }

#if defined(CONFIG_F22_DEBUG_PROBE_BLINK)
    k_timer_start(&blink_timer, K_NO_WAIT, K_MSEC(250));
#endif

#if defined(CONFIG_F22_DEBUG_PROBE_USB_STATE)
    set_usb_pattern(USB_PROBE_PATTERN_ARMED);
    k_timer_start(&usb_state_timer, K_NO_WAIT, K_MSEC(150));

    ret = usb_enable(usb_status_cb);
    if (ret != 0) {
        atomic_set(&usb_enable_failed, 1);
        (void)gpio_pin_set_dt(&debug_led, 1);
    }
#endif

    return 0;
}

SYS_INIT(f22_debug_probe_init, APPLICATION, 99);