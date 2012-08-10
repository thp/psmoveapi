
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include "psmove.h"
#include "psmove_calibration.h"

#include "xdo.h"

/* Scaling factor to convert gyro values to relative pixel coordinates */
#define SCALE(v) (14*v)

/* Macro to generate code to map a PS Move button to a mouse button */
#define MAP_PSMOVE_TO_MOUSE(psm, mou, mod) \
    { \
            if (pressed & (psm)) { \
                if (mod) xdo_keysequence_down(xdo, CURRENTWINDOW, (mod), 0); \
                xdo_mousedown(xdo, CURRENTWINDOW, (mou)); \
            } else if (released & (psm)) { \
                xdo_mouseup(xdo, CURRENTWINDOW, (mou)); \
                if (mod) xdo_keysequence_up(xdo, CURRENTWINDOW, (mod), 0); \
            } \
    }

#define BAILOUT(msg) \
    { \
        fprintf(stderr, "%s\n", msg); \
        return 1; \
    }

int
main(int argc, char *argv[])
{
    PSMove *move = psmove_connect();
    if (!move) {
        BAILOUT("Cannot connect to move controller.");
    }

    PSMoveCalibration *calibration = psmove_calibration_new(move);
    if (!calibration) {
        BAILOUT("Cannot instantiate calibration.");
    }

    if (!psmove_calibration_supported(calibration)) {
        BAILOUT("Calibration data not found. Pair using USB first.");
    }

    xdo_t *xdo = xdo_new(NULL);
    if (!xdo) {
        BAILOUT("Cannot create new xdo instance.");
    }

    int old_buttons = 0;

    while (1) {
        while (psmove_poll(move)) {
            int input[6] = { 0, 0, 0, 0, 0, 0 };
            float output[6];

            int buttons = psmove_get_buttons(move);
            int pressed = (buttons ^ old_buttons) & buttons;
            int released = (buttons ^ old_buttons) & old_buttons;

            /**
             * Default button mapping:
             *  - Cross x ....... left mouse button
             *  - Move .......... middle mouse button
             *  - Circle o ...... right mouse button
             **/
            MAP_PSMOVE_TO_MOUSE(Btn_CROSS, 1, NULL);
            MAP_PSMOVE_TO_MOUSE(Btn_MOVE, 2, NULL);
            MAP_PSMOVE_TO_MOUSE(Btn_CIRCLE, 3, NULL);

            /**
             * Additional mappings (depends on WM):
             *  - Square [] ..... move window under cursor
             *  - Triangle /\ ... resize window under cursor
             **/
            MAP_PSMOVE_TO_MOUSE(Btn_SQUARE, 1, "alt");
            MAP_PSMOVE_TO_MOUSE(Btn_TRIANGLE, 2, "alt");

            old_buttons = buttons;

            psmove_get_gyroscope(move, &input[3], &input[4], &input[5]);
            psmove_calibration_map(calibration, input, output, 6);

            int dx = SCALE(-output[5]);
            int dy = SCALE(-output[3]);

            if (dx || dy) {
                xdo_mousemove_relative(xdo, dx, dy);
            }
        }

        /* Controller sends 60 reports / second - wait for the next one */
        usleep(1000000/60);
    }

    xdo_free(xdo);

    psmove_calibration_free(calibration);
    psmove_disconnect(move);

    return 0;
}

