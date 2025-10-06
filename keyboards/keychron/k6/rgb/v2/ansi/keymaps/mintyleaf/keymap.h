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
#include QMK_KEYBOARD_H
enum BT_keycodes { BT_PROFILE1 = SAFE_RANGE, BT_PROFILE2, BT_PROFILE3, BT_PAIR, BT_TOGGLE, BT_RESET, BT_BATTERY, K6_SAFE_RANGE };
enum layer_names {
    WIN_BASE,
    WIN_FN1,
    MAC_BASE,
    MAC_FN1,
    FN2,
};

#define KC_TASK LGUI(KC_TAB)       // Task viewer
#define KC_FLXP LGUI(KC_E)         // Windows file explorer
#define KC_MCTL KC_MISSION_CONTROL // Mission Control
#define KC_LPAD KC_LAUNCHPAD       // Launchpad