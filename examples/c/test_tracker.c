
#include <stdio.h>

#include "psmove.h"

#if defined(PSMOVE_HAVE_TRACKER)

#include <time.h>
#include <unistd.h>
#include <assert.h>

#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"

#include "psmove.h"
#include "psmove_tracker.h"

int main(int arg, char** args) {
    int i;
    int count = psmove_count_connected();
    PSMove* controllers[count];

    printf("%s", "### Trying to init PSMoveTracker...");
    PSMoveTracker* tracker = psmove_tracker_new();
    printf("%s\n", "OK");
    printf("### Found %d controllers.\n", count);

    void *frame;
    unsigned char r, g, b;
    int result;

    for (i=0; i<count; i++) {
        printf("Opening controller %d\n", i);
        controllers[i] = psmove_connect_by_id(i);
        assert(controllers[i] != NULL);

        while (1) {
            printf("Calibrating controller %d...", i);
            fflush(stdout);
            result = psmove_tracker_enable(tracker, controllers[i]);

            if (result == Tracker_CALIBRATED) {
                printf("OK\n");
                break;
            } else {
                printf("ERROR\n");
                return 1;
            }
        }

    }

    while (1) {
        if ((cvWaitKey(10) & 255) == 27) {
            break;
        }

        psmove_tracker_update_image(tracker);
        psmove_tracker_update(tracker, NULL);

        frame = psmove_tracker_get_image(tracker);
        if (frame) {
            cvShowImage("live camera feed", frame);
        }

        for (i=0; i<count; i++) {
            psmove_tracker_get_color(tracker, controllers[i], &r, &g, &b);
            psmove_set_leds(controllers[i], r, g, b);
            psmove_update_leds(controllers[i]);

            float x, y, r;
            psmove_tracker_get_position(tracker, controllers[i], &x, &y, &r);
            printf("x: %10.2f, y: %10.2f, r: %10.2f\n", x, y, r);
        }
    }

    for (i=0; i<count; i++) {
        psmove_disconnect(controllers[i]);
    }

    psmove_tracker_free(tracker);
    return 0;
}

#else /* PSMOVE_HAVE_TRACKER */

int main()
{
    printf("PS Move API compiled without Tracker support.\n");
    return 1;
}

#endif
