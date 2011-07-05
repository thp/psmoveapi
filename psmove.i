%module psmove
%{

 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (C) 2011 Thomas Perl <m@thp.io>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 **/

#include "psmove.h"

%}

%include "psmove.h"

typedef struct {} PSMove;

int count_connected();

%extend PSMove {
    /* Connection type */
    const int connection_type;

    /* Accelerometer */
    const int ax;
    const int ay;
    const int az;

    /* Gyroscope */
    const int gx;
    const int gy;
    const int gz;

    /* Magnetometer */
    const int mx;
    const int my;
    const int mz;

    PSMove(int id=0) {
        return psmove_connect_by_id(id);
    }

    void set_leds(int r, int g, int b) {
        psmove_set_leds($self, r, g, b);
    }

    void set_rumble(int rumble) {
        psmove_set_rumble($self, rumble);
    }

    int update_leds() {
        return psmove_update_leds($self);
    }

    int pair() {
        return psmove_pair($self);
    }

    int poll() {
        return psmove_poll($self);
    }

    int get_buttons() {
        return psmove_get_buttons($self);
    }

    int get_battery() {
        return psmove_get_battery($self);
    }

    int get_trigger() {
        return psmove_get_trigger($self);
    }

    ~PSMove() {
        psmove_disconnect($self);
    }

}


%{

int
PSMove_connection_type_get(PSMove *move) {
    return psmove_connection_type(move);
}

int
PSMove_ax_get(PSMove *move) {
    int result;
    psmove_get_accelerometer(move, &result, NULL, NULL);
    return result;
}

int
PSMove_ay_get(PSMove *move) {
    int result;
    psmove_get_accelerometer(move, NULL, &result, NULL);
    return result;
}

int
PSMove_az_get(PSMove *move) {
    int result;
    psmove_get_accelerometer(move, NULL, NULL, &result);
    return result;
}

int
PSMove_gx_get(PSMove *move) {
    int result;
    psmove_get_gyroscope(move, &result, NULL, NULL);
    return result;
}

int
PSMove_gy_get(PSMove *move) {
    int result;
    psmove_get_gyroscope(move, NULL, &result, NULL);
    return result;
}

int
PSMove_gz_get(PSMove *move) {
    int result;
    psmove_get_gyroscope(move, NULL, NULL, &result);
    return result;
}


int
PSMove_mx_get(PSMove *move) {
    int result;
    psmove_get_magnetometer(move, &result, NULL, NULL);
    return result;
}

int
PSMove_my_get(PSMove *move) {
    int result;
    psmove_get_magnetometer(move, NULL, &result, NULL);
    return result;
}

int
PSMove_mz_get(PSMove *move) {
    int result;
    psmove_get_magnetometer(move, NULL, NULL, &result);
    return result;
}

int count_connected()
{
    return psmove_count_connected();
}

%}


