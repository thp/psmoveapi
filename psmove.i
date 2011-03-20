%module psmove
%{

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

    /* FIXME: set/get btaddr */

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

    int poll() {
        return psmove_poll($self);
    }

    int get_buttons() {
        return psmove_get_buttons($self);
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


