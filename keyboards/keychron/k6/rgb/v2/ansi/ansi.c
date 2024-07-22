/*
Copyright 2024 mintyleaf

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
#ifdef BLUETOOTH_ENABLE
#    include "iton_bt.h"
#    include "outputselect.h"

uint32_t last_update_time = 0;

uint32_t ev_connected     = 0;
uint32_t ev_connecting    = 0;
uint32_t ev_pairing       = 0;
uint32_t ev_disconnected  = 0;
uint32_t ev_battery_level = 0;

uint32_t battery_level = 0;
uint32_t bt_profile    = 0;

bool bluetooth_dip_switch = false;

void iton_bt_connection_successful() {
    set_output(OUTPUT_BLUETOOTH);
    ev_connected  = 1500;
    ev_pairing    = 0;
    ev_connecting = 0;
}

void iton_bt_entered_pairing() {
    ev_pairing    = 1;
    ev_connected  = 0;
    ev_connecting = 0;
}

void iton_bt_enters_connection_state() {
    ev_connecting = 1;
    ev_connected  = 0;
    ev_pairing    = 0;
}

void iton_bt_disconnected() {
    set_output(OUTPUT_USB);
    ev_disconnected = 1500;
    ev_connected    = 0;
    ev_pairing      = 0;
    ev_connecting   = 0;
}

void iton_bt_battery_level(uint8_t level) {
    battery_level    = level;
    ev_battery_level = 1500;
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (bluetooth_dip_switch && record->event.pressed) {
        switch (keycode) {
            case BT_PROFILE1:
                iton_bt_switch_profile(0);
                bt_profile = 0;
                break;
            case BT_PROFILE2:
                iton_bt_switch_profile(1);
                bt_profile = 1;
                break;
            case BT_PROFILE3:
                iton_bt_switch_profile(2);
                bt_profile = 2;
                break;
            case BT_PAIR:
                iton_bt_enter_pairing();
                break;
            case BT_RESET:
                iton_bt_reset_pairing();
                break;
            case BT_BATTERY:
                iton_bt_query_battery_level();
                break;
            default:
                break;
        }
    }
    return process_record_user(keycode, record);
}
#endif

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    uint8_t layer = get_highest_layer(layer_state);
    if (layer != 0 && layer != 2) {
        for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
            for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
                uint8_t index = g_led_config.matrix_co[row][col];

                if (index >= led_min && index < led_max && index != NO_LED && keymap_key_to_keycode(layer, (keypos_t){col, row}) > KC_TRNS) {
                    rgb_matrix_set_color(index, RGB_WHITE);
                }
            }
        }
    }

#ifdef BLUETOOTH_ENABLE
    if (!bluetooth_dip_switch) {
        return false;
    }

    uint32_t current_time = timer_read();                    // Get the current time in milliseconds
    uint32_t elapsed      = current_time - last_update_time; // Calculate elapsed time

    // Update the last update time for the next call
    last_update_time = current_time;

    if (ev_connected > 0 && elapsed < ev_connected) {
        uint8_t profile_index = 16 + bt_profile;
        if ((ev_connected / 250) % 2 == 0) {
            rgb_matrix_set_color(profile_index, RGB_GREEN);
        } else {
            rgb_matrix_set_color(profile_index, RGB_OFF);
        }
        ev_connected -= elapsed;
    }

    if (ev_connecting > 0) {
        uint8_t profile_index = 16 + bt_profile;
        if ((current_time / 125) % 2 == 0) {
            rgb_matrix_set_color(profile_index, RGB_YELLOW);
        } else {
            rgb_matrix_set_color(profile_index, RGB_OFF);
        }
    }

    if (ev_pairing > 0) {
        uint8_t profile_index = 16 + bt_profile;
        if ((current_time / 62) % 2 == 0) {
            rgb_matrix_set_color(profile_index, RGB_BLUE);
        } else {
            rgb_matrix_set_color(profile_index, RGB_OFF);
        }
    }

    if (ev_disconnected > 0 && elapsed < ev_disconnected) {
        uint8_t profile_index = 16 + bt_profile;
        if ((ev_disconnected / 250) % 2 == 0) {
            rgb_matrix_set_color(profile_index, RGB_RED);
        } else {
            rgb_matrix_set_color(profile_index, RGB_OFF);
        }
        ev_disconnected -= elapsed;
    }

    if (ev_battery_level > 0 && elapsed < ev_battery_level) {
        ev_battery_level -= elapsed;
        int total_leds       = MATRIX_ROWS * MATRIX_COLS;
        int leds_per_section = total_leds / 4;
        int leds_to_light_up = leds_per_section * battery_level;

        for (int i = 0; i < total_leds; i++) {
            if (i < leds_to_light_up && i >= led_min && i < led_max && i != NO_LED) {
                if (battery_level == 4) {
                    rgb_matrix_set_color(i, RGB_GREEN);
                } else if (battery_level == 3) {
                    rgb_matrix_set_color(i, RGB_YELLOW);
                } else if (battery_level == 2) {
                    rgb_matrix_set_color(i, RGB_ORANGE);
                } else if (battery_level == 1) {
                    rgb_matrix_set_color(i, RGB_RED);
                } else {
                    rgb_matrix_set_color(i, RGB_WHITE);
                }
            }
        }
    }

#endif
    return false;
}

bool dip_switch_update_user(uint8_t index, bool active) {
    switch (index) {
        case 1:
            if (active) {
                layer_move(MAC_BASE);
            } else {
                layer_move(WIN_BASE);
            }
            break;
#ifdef BLUETOOTH_ENABLE
        case 0:
            // dip switch inactive in bt state
            bluetooth_dip_switch = !active;
            break;
#endif
    }
    return true;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KC_MISSION_CONTROL:
            if (record->event.pressed) {
                host_consumer_send(0x29F);
            } else {
                host_consumer_send(0);
            }
            return false;
        case KC_LAUNCHPAD:
            if (record->event.pressed) {
                host_consumer_send(0x2A0);
            } else {
                host_consumer_send(0);
            }
            return false;
        default:
            return true;
    }
}

void keyboard_post_init_user(void) {
    // Customise these values to desired behaviour
    // debug_enable = true;
    // debug_matrix   = true;
    // debug_keyboard = true;
    // debug_mouse=true;
}