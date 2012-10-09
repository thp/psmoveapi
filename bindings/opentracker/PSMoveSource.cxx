/* ========================================================================
 *
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
 * Released under the same terms as the OpenTracker library itself.
 *
 * Based on example code from:
 * Copyright (c) 2006,
 * Institute for Computer Graphics and Vision
 * Graz University of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the Graz University of Technology nor the names of
 * its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ========================================================================
 * PROJECT: OpenTracker
 * ======================================================================== */

#include <OpenTracker/input/PSMoveSource.h>

#ifdef USE_PSMOVE

#include "psmove.h"
#include "psmove_tracker.h"

namespace ot {
    PSMoveSource::PSMoveSource(PSMoveTracker *tracker, int controller)
        : Node(),
          move(psmove_connect_by_id(controller)),
          tracker(tracker)
    {
        psmove_enable_orientation(move, PSMove_True);

        while (psmove_tracker_enable(tracker, move) != Tracker_CALIBRATED) {
            // Wait until calibration is done
        }
    }

    PSMoveSource::~PSMoveSource()
    {
        psmove_tracker_disable(tracker, move);
        psmove_disconnect(move);
    }

    Event &
    PSMoveSource::getEvent()
    {
        if (move != NULL) {
            /**
             * psmove_tracker_update_image() and
             * psmove_tracker_update() are called by PSMoveModule,
             * so we don't need to call them here
             **/

            // Read the latest input reports from the controller
            while (psmove_poll(move));

            // Button and trigger status from the controller
            event.getButton() = psmove_get_buttons(move);
            event.setAttribute("trigger", psmove_get_trigger(move));

            // 3D position (FIXME: This requires more work ;)
            float x, y, radius;
            psmove_tracker_get_position(tracker, move, &x, &y, &radius);

            std::vector<float> position(3);
            position[0] = x;
            position[1] = y;
            position[2] = radius;
            event.getPosition() = position;

            // 3D orientation
            float q0, q1, q2, q3;
            psmove_get_orientation(move, &q0, &q1, &q2, &q3);

            std::vector<float> quaternion(4);
            quaternion[0] = q0;
            quaternion[1] = q1;
            quaternion[2] = q2;
            quaternion[3] = q3;
            event.getOrientation() = quaternion;
        }

        return event;
    }

} /* namespace ot */

#endif /* USE_PSMOVE */
