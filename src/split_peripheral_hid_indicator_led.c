#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>

#include <dt-bindings/zmk/hid_indicators.h>
#include <zmk/event_manager.h>
#include <zmk/events/hid_indicators_changed.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) && \
    DT_NODE_HAS_STATUS(DT_ALIAS(led0), okay)

static const struct gpio_dt_spec caps_lock_led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static int split_peripheral_hid_indicator_led_init(void)
{
    if (!gpio_is_ready_dt(&caps_lock_led)) {
        return -ENODEV;
    }

    return gpio_pin_configure_dt(&caps_lock_led, GPIO_OUTPUT_INACTIVE);
}

static int split_peripheral_hid_indicator_led_listener(const zmk_event_t *event)
{
    const struct zmk_hid_indicators_changed *indicators = as_zmk_hid_indicators_changed(event);

    if (indicators == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    gpio_pin_set_dt(&caps_lock_led, (indicators->indicators & HID_INDICATOR_CAPS_LOCK) != 0);
    return ZMK_EV_EVENT_BUBBLE;
}

SYS_INIT(split_peripheral_hid_indicator_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

ZMK_LISTENER(split_peripheral_hid_indicator_led, split_peripheral_hid_indicator_led_listener);
ZMK_SUBSCRIPTION(split_peripheral_hid_indicator_led, zmk_hid_indicators_changed);

#endif