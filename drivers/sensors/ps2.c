/*
Copyright 2024 Jonas Haseleu <jonas@holykeebs.com>

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

#include "ps2.h"

#include "drivers/ps2/ps2_mouse.h"

static report_mouse_t ps2_driver_get_report(report_mouse_t mouse_report) {
    ps2_mouse_read(&mouse_report);
    return mouse_report;
}

static uint16_t ps2_driver_get_cpi(void) {
    return 0;
}

static void ps2_driver_set_cpi(uint16_t cpi) {
    (void)cpi;
}

static bool ps2_driver_init(void) {
    ps2_mouse_init();
    return true;
}

const pointing_device_driver_t ps2_pointing_device_driver = {
    .init       = ps2_driver_init,
    .get_report = ps2_driver_get_report,
    .set_cpi    = ps2_driver_set_cpi,
    .get_cpi    = ps2_driver_get_cpi,
};
