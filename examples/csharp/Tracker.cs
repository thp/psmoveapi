
using System;
using io.thp.psmove;

public class Tracker {
    public static int Main(string [] args) {
        PSMoveTracker tracker = new PSMoveTracker();
        PSMove move = new PSMove();

        /* Calibrate the controller in the camera picture */
        while (tracker.enable(move) != Status.Tracker_CALIBRATED);

        while (true) {
            /* Get a new image from the camera */
            tracker.update_image();
            /* Track controllers in the camera picture */
            tracker.update();

            /* Optional and not required by default
            byte r, g, b;
            tracker.get_color(move, out r, out g, out b);
            move.set_leds(r, g, b);
            move.update_leds();
            */

            /* If we're tracking, output the position */
            if (tracker.get_status(move) == Status.Tracker_TRACKING) {
                float x, y, radius;
                tracker.get_position(move, out x, out y, out radius);
                Console.WriteLine(string.Format("Tracking: x:{0:0.000}, " +
                            "y:{1:0.000}, radius:{2:0.000}", x, y, radius));
            } else {
                Console.WriteLine("Not Tracking!");
            }
        }

        return 0;
    }
};

