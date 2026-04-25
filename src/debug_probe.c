#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#if defined(CONFIG_F22_DEBUG_PROBE_USB_STATE) || \
    defined(CONFIG_F22_DEBUG_PROBE_USB_ARMED) || \
    defined(CONFIG_F22_DEBUG_PROBE_USB_RAW) || \
    defined(CONFIG_F22_DEBUG_PROBE_USB_ENABLE_GATE)
#define F22_DEBUG_PROBE_HAS_USB 1
#endif

#if defined(F22_DEBUG_PROBE_HAS_USB)
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

#if defined(F22_DEBUG_PROBE_HAS_USB)
enum usb_probe_pattern {
    USB_PROBE_PATTERN_OFF = 0,
    USB_PROBE_PATTERN_ARMED = 1,
    USB_PROBE_PATTERN_BEFORE_ENABLE = 2,
    USB_PROBE_PATTERN_AFTER_ENABLE = 3,
    USB_PROBE_PATTERN_RESET = 4,
    USB_PROBE_PATTERN_CONNECTED = 5,
    USB_PROBE_PATTERN_CONFIGURED = 6,
    USB_PROBE_PATTERN_DISCONNECTED = 7,
    USB_PROBE_PATTERN_SUSPENDED = 8,
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

#if defined(CONFIG_F22_DEBUG_PROBE_USB_INIT_BRACKET)
static int f22_debug_probe_pre_usb_init(void)
{
    int ret;

    ret = configure_debug_led();
    if (ret != 0) {
        return ret;
    }

    return gpio_pin_set_dt(&debug_led, 0);
}

static int f22_debug_probe_post_usb_init(void)
{
    int ret;

    ret = configure_debug_led();
    if (ret != 0) {
        return ret;
    }

    return gpio_pin_set_dt(&debug_led, 1);
}

SYS_INIT(f22_debug_probe_pre_usb_init, POST_KERNEL, 49);
SYS_INIT(f22_debug_probe_post_usb_init, POST_KERNEL, 51);
#endif

#if defined(CONFIG_F22_DEBUG_PROBE_USB_INIT_LADDER) || \
    defined(CONFIG_F22_DEBUG_PROBE_USB_APP_LADDER)
static void ladder_pulse(int n)
{
    /* Brief OFF/ON pulses so each group is countable, then a long ON hold
     * so successive priority groups are visually separated. The LED is
     * left ON between groups; if boot stalls, the LED stays solid ON and
     * the last group of pulses observed identifies the last priority that
     * ran to completion.
     */
    for (int i = 0; i < n; i++) {
        (void)gpio_pin_set_dt(&debug_led, 0);
        k_msleep(120);
        (void)gpio_pin_set_dt(&debug_led, 1);
        k_msleep(120);
    }
    k_msleep(900);
}

#define F22_LADDER_STEP(name, level, prio, count)                  \
    static int f22_ladder_##name(void)                             \
    {                                                              \
        int ret = configure_debug_led();                           \
        if (ret != 0) {                                            \
            return ret;                                            \
        }                                                          \
        (void)gpio_pin_set_dt(&debug_led, 1);                      \
        ladder_pulse(count);                                       \
        return 0;                                                  \
    }                                                              \
    SYS_INIT(f22_ladder_##name, level, prio)
#endif

#if defined(CONFIG_F22_DEBUG_PROBE_USB_INIT_LADDER)
F22_LADDER_STEP(pk_52,  POST_KERNEL, 52, 1);
F22_LADDER_STEP(pk_60,  POST_KERNEL, 60, 2);
F22_LADDER_STEP(pk_70,  POST_KERNEL, 70, 3);
F22_LADDER_STEP(pk_80,  POST_KERNEL, 80, 4);
F22_LADDER_STEP(pk_90,  POST_KERNEL, 90, 5);
F22_LADDER_STEP(pk_99,  POST_KERNEL, 99, 6);
F22_LADDER_STEP(app_01, APPLICATION,  1, 7);
F22_LADDER_STEP(app_50, APPLICATION, 50, 8);
F22_LADDER_STEP(app_98, APPLICATION, 98, 9);
#endif

#if defined(CONFIG_F22_DEBUG_PROBE_USB_APP_LADDER)
F22_LADDER_STEP(app_51, APPLICATION, 51, 1);
F22_LADDER_STEP(app_80, APPLICATION, 80, 2);
F22_LADDER_STEP(app_89, APPLICATION, 89, 3);
F22_LADDER_STEP(app_91, APPLICATION, 91, 4);
F22_LADDER_STEP(app_94, APPLICATION, 94, 5);
F22_LADDER_STEP(app_95, APPLICATION, 95, 6);
F22_LADDER_STEP(app_96, APPLICATION, 96, 7);
F22_LADDER_STEP(app_97, APPLICATION, 97, 8);
F22_LADDER_STEP(app_98, APPLICATION, 98, 9);
#endif

#if defined(CONFIG_F22_DEBUG_PROBE_USB_ENABLE_GATE)
#define ENABLE_GATE_LEAD_MS 1000
#define ENABLE_GATE_PULSE_MS 350
#define ENABLE_GATE_HOLD_MS 1200

extern void usb_status_cb(enum usb_dc_status_code status, const uint8_t *params);

static void enable_gate_pulse(int n)
{
    (void)gpio_pin_set_dt(&debug_led, 0);
    k_msleep(ENABLE_GATE_LEAD_MS);

    for (int i = 0; i < n; i++) {
        (void)gpio_pin_set_dt(&debug_led, 1);
        k_msleep(ENABLE_GATE_PULSE_MS);
        (void)gpio_pin_set_dt(&debug_led, 0);
        k_msleep(ENABLE_GATE_PULSE_MS);
    }

    k_msleep(ENABLE_GATE_LEAD_MS);
    (void)gpio_pin_set_dt(&debug_led, 1);
    k_msleep(ENABLE_GATE_HOLD_MS);
}

static int f22_debug_probe_usb_enable_gate_before_hid(void)
{
    int ret;

    ret = configure_debug_led();
    if (ret != 0) {
        return ret;
    }

    enable_gate_pulse(1);
    return 0;
}

static int f22_debug_probe_usb_enable_gate(void)
{
    unsigned int key;
    int ret;

    ret = configure_debug_led();
    if (ret != 0) {
        return ret;
    }

    enable_gate_pulse(2);
    key = irq_lock();
    ret = usb_enable(usb_status_cb);
    irq_unlock(key);

    if (ret != 0) {
        enable_gate_pulse(3);
        return ret;
    }

    enable_gate_pulse(4);
    k_msleep(2000);
    enable_gate_pulse(5);

    return 0;
}

SYS_INIT(f22_debug_probe_usb_enable_gate_before_hid, APPLICATION, 94);
SYS_INIT(f22_debug_probe_usb_enable_gate, APPLICATION, 98);
#endif

#if defined(CONFIG_F22_DEBUG_PROBE_USB_RAW)
#define USB_RAW_THREAD_STACK 1024
#define USB_RAW_THREAD_PRIO 7

static K_THREAD_STACK_DEFINE(usb_raw_stack, USB_RAW_THREAD_STACK);
static struct k_thread usb_raw_thread_data;

static void usb_raw_thread(void *p1, void *p2, void *p3)
{
    int ret;

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    set_usb_pattern(USB_PROBE_PATTERN_ARMED);
    k_msleep(3000);
    set_usb_pattern(USB_PROBE_PATTERN_BEFORE_ENABLE);
    k_msleep(1500);

    ret = usb_enable(usb_status_cb);
    if (ret != 0) {
        atomic_set(&usb_enable_failed, 1);
        return;
    }

    set_usb_pattern(USB_PROBE_PATTERN_AFTER_ENABLE);
}
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

#if defined(F22_DEBUG_PROBE_HAS_USB)
    set_usb_pattern(USB_PROBE_PATTERN_ARMED);
    k_timer_start(&usb_state_timer, K_NO_WAIT, K_MSEC(150));
#endif

#if defined(CONFIG_F22_DEBUG_PROBE_USB_STATE)
    ret = usb_enable(usb_status_cb);
    if (ret != 0) {
        atomic_set(&usb_enable_failed, 1);
        (void)gpio_pin_set_dt(&debug_led, 1);
    }
#endif

#if defined(CONFIG_F22_DEBUG_PROBE_USB_RAW)
    (void)k_thread_create(&usb_raw_thread_data, usb_raw_stack,
                          K_THREAD_STACK_SIZEOF(usb_raw_stack),
                          usb_raw_thread, NULL, NULL, NULL,
                          USB_RAW_THREAD_PRIO, 0, K_NO_WAIT);
#endif

    return 0;
}

#if !defined(CONFIG_F22_DEBUG_PROBE_USB_ENABLE_GATE)
SYS_INIT(f22_debug_probe_init, APPLICATION, 99);
#endif