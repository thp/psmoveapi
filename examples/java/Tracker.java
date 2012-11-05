 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 **/

import io.thp.psmove.PSMove;
import io.thp.psmove.Frame;
import io.thp.psmove.psmoveapi;

import io.thp.psmove.PSMoveTracker;
import io.thp.psmove.Status;

public class Tracker {
    public static void main(String [] args) {
        int connected = psmoveapi.count_connected();
        System.out.println("Connected controllers: " + connected);

        if (connected == 0) {
            System.out.println("Please connect a controller first.");
            System.exit(1);
        }

        PSMoveTracker tracker = new PSMoveTracker();
        PSMove move = new PSMove();

        // Mirror the camera image
        tracker.set_mirror(1);

        while (tracker.enable(move) != Status.Tracker_CALIBRATED);

        while (true) {
            tracker.update_image();
            tracker.update();

            /* Optional and not required by default
            short [] r = {0};
            short [] g = {0};
            short [] b = {0};
            tracker.get_color(move, r, g, b);
            move.set_leds(r[0], g[0], b[0]);
            move.update_leds();
            */

            if (tracker.get_status(move) == Status.Tracker_TRACKING) {
                float [] x = { 0.f };
                float [] y = { 0.f };
                float [] radius = { 0.f };

                tracker.get_position(move, x, y, radius);

                System.out.format("x: %5.2f y: %5.2f radius: %5.2f\n",
                        x[0], y[0], radius[0]);
            } else {
                System.out.println("Not tracking.");
            }
        }
    }
}

