/*
Copyright 2025 mintyleaf

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "quantum.h"
#include "keymap.h"
#include <stdint.h>

#ifdef BLUETOOTH_ENABLE
#    include "iton_bt.h"
#    include "outputselect.h"
#endif

#define STARTUP_FLASH_DURATION_MS 1500
static uint32_t startup_flash = STARTUP_FLASH_DURATION_MS;

static uint32_t last_update_time = 0;

#ifdef BLUETOOTH_ENABLE
#    define BT_CONNECTION_SUCCESSFUL_DURATION_MS 2500
#    define BT_DISCONNECTED_DURATION_MS 2500
#    define BT_BATTERY_DURATION_MS 2500
#    define BT_BATTERY_WAIT_QUERY_DURATION_MS 10000

#    define BT_PROFILE_LED_START_INDEX 16
#    define BATTERY_LED_INDEX 49

#    define BT_PAIRING_BLINK_MS 62
#    define BT_CONNECTING_BLINK_MS 125
#    define BT_DISCONNECTED_BLINK_MS 250

#    define NUM_BATTERY_LEVELS (sizeof(BATTERY_COLOR_MAP) / sizeof(BATTERY_COLOR_MAP[0]))

typedef enum {
    BATTERY_LEVEL_NONE = 0,
    BATTERY_LEVEL_CRITICAL = 1,
    BATTERY_LEVEL_LOW = 2,
    BATTERY_LEVEL_MEDIUM = 3,
    BATTERY_LEVEL_FULL = 4
} battery_level_t;

const rgb_t BATTERY_COLOR_MAP[] = {
    {RGB_WHITE},
    {RGB_RED},
    {RGB_ORANGE},
    {RGB_YELLOW},
    {RGB_GREEN}
};
#endif

#ifdef BLUETOOTH_ENABLE
static bool     ev_connecting_flag     = false;
static bool     ev_pairing_flag        = false;
static uint32_t ev_disconnected_timer  = 0;
static uint32_t ev_connected_timer     = 0;
static uint32_t ev_battery_level_timer = 0;

static uint32_t battery_level = 0;
static uint32_t bt_profile    = 0;

static bool bluetooth_dip_switch = false;

static void set_profile_led_blinking(uint32_t current_time, uint32_t blink_ms, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t profile_index = BT_PROFILE_LED_START_INDEX + bt_profile;
    if ((current_time / blink_ms) % 2 == 0) {
        rgb_matrix_set_color(profile_index, r, g, b);
    }
}

void iton_bt_connection_successful() {
    set_output(OUTPUT_BLUETOOTH);
    ev_connected_timer = BT_CONNECTION_SUCCESSFUL_DURATION_MS;
    ev_pairing_flag    = false;
    ev_connecting_flag = false;
}

void iton_bt_entered_pairing() {
    ev_pairing_flag    = true;
    ev_connected_timer = 0;
    ev_connecting_flag = false;
}

void iton_bt_enters_connection_state() {
    ev_connecting_flag = true;
    ev_connected_timer = 0;
    ev_pairing_flag    = false;
}

void iton_bt_disconnected() {
    ev_disconnected_timer = BT_DISCONNECTED_DURATION_MS;
    ev_connected_timer    = 0;
    ev_pairing_flag       = false;
    ev_connecting_flag    = false;
}

void iton_bt_battery_level(uint8_t level) {
    battery_level          = level;
    ev_battery_level_timer = BT_BATTERY_DURATION_MS;
}
#endif

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
#ifdef BLUETOOTH_ENABLE
    if (record->event.pressed && bluetooth_dip_switch) {
        switch (keycode) {
            case BT_PROFILE1:
            case BT_PROFILE2:
            case BT_PROFILE3:
                {
                    uint8_t profile_idx = keycode - BT_PROFILE1;
                    iton_bt_switch_profile(profile_idx);
                    bt_profile = profile_idx;
                }
                return false;
            case BT_PAIR:
                iton_bt_enter_pairing();
                return false;
            case BT_RESET:
                iton_bt_reset_pairing();
                return false;
            case BT_BATTERY:
                ev_battery_level_timer = BT_BATTERY_WAIT_QUERY_DURATION_MS;
                iton_bt_query_battery_level();
                return false;
            default:
                break;
        }
    }
#endif
    switch (keycode) {
        case KC_MISSION_CONTROL:
            host_consumer_send(record->event.pressed ? 0x29F : 0);
            return false;
        case KC_LAUNCHPAD:
            host_consumer_send(record->event.pressed ? 0x2A0 : 0);
            return false;
        default:
            break;
    }
    return process_record_user(keycode, record);
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    uint32_t current_time = timer_read();
    uint32_t elapsed      = (current_time >= last_update_time) ? (current_time - last_update_time) : 1;
    last_update_time      = current_time;

    if (startup_flash > 0) {
        hsv_t flash     = (hsv_t){HSV_WHITE};
        flash.v         = (uint8_t)((startup_flash * 255UL) / STARTUP_FLASH_DURATION_MS);
        rgb_t flash_rgb = hsv_to_rgb(flash);
        rgb_matrix_set_color_all(flash_rgb.r, flash_rgb.g, flash_rgb.b);
        if (elapsed >= startup_flash) {
            startup_flash = 0;
        } else {
            startup_flash -= elapsed;
        }
        return true;
    }

    uint8_t layer = get_highest_layer(layer_state);
    if (layer != 0 && layer != 2) {
        for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
            for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
                uint8_t index = g_led_config.matrix_co[row][col];

                if (index >= led_min && index < led_max && index != NO_LED) {
                    if (keymap_key_to_keycode(layer, (keypos_t){col, row}) > KC_TRNS) {
                        rgb_matrix_set_color(index, RGB_WHITE);
                    } else {
                        // fix non-white highlight at some rows when leds is inactive by default
                        rgb_matrix_set_color(index, 0x01, 0x01, 0x01);
                    }
                }
            }
        }
    }

#ifdef BLUETOOTH_ENABLE
    if (!bluetooth_dip_switch) {
        return true;
    }

    if (ev_pairing_flag) {
        set_profile_led_blinking(current_time, BT_PAIRING_BLINK_MS, RGB_BLUE);
    } else if (ev_connecting_flag) {
        set_profile_led_blinking(current_time, BT_CONNECTING_BLINK_MS, RGB_BLUE);
    } else if (ev_connected_timer > 0) {
        rgb_matrix_set_color(BT_PROFILE_LED_START_INDEX + bt_profile, RGB_WHITE);
    } else if (ev_disconnected_timer > 0) {
        set_profile_led_blinking(current_time, BT_DISCONNECTED_BLINK_MS, RGB_RED);
    }

    if (ev_connected_timer > elapsed)
        ev_connected_timer -= elapsed;
    else
        ev_connected_timer = 0;
    if (ev_disconnected_timer > elapsed)
        ev_disconnected_timer -= elapsed;
    else
        ev_disconnected_timer = 0;

    if (ev_battery_level_timer > 0) {
        rgb_t color = BATTERY_COLOR_MAP[battery_level];
        rgb_matrix_set_color(BATTERY_LED_INDEX, color.r, color.g, color.b);

        if (ev_battery_level_timer > elapsed) {
            ev_battery_level_timer -= elapsed;
        } else {
            ev_battery_level_timer = 0;
            battery_level          = BATTERY_LEVEL_NONE;
        }
    }
#endif
    return true;
}

bool dip_switch_update_user(uint8_t index, bool active) {
    switch (index) {
        case 1:
            layer_move(active ? MAC_BASE : WIN_BASE);
            return false;
#ifdef BLUETOOTH_ENABLE
        case 0:
            // dip switch is inactive in bt state
            if (!active) {
                set_output(OUTPUT_NONE);
                iton_bt_init();
            } else {
                iton_bt_deinit();
                set_output(OUTPUT_USB);
            }
            bluetooth_dip_switch = !active;
            return false;
#endif
    }
    return true;
}
