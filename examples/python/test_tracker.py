
#
# PS Move API - An interface for the PS Move Motion Controller
# Copyright (c) 2012 Thomas Perl <m@thp.io>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#    1. Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#    2. Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#


import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'build'))

import psmove

if psmove.count_connected() < 1:
    print('No controller connected')
    sys.exit(1)

tracker = psmove.PSMoveTracker()
move = psmove.PSMove()

if move.connection_type != psmove.Conn_Bluetooth:
    print('Please connect controller via Bluetooth')
    sys.exit(1)

# Mirror the camera image
tracker.set_mirror(True)

# Calibrate the controller with the tracker
result = -1
while result != psmove.Tracker_CALIBRATED:
    print 'Trying to calibrate...'
    result = tracker.enable(move)

auto_update_leds = tracker.get_auto_update_leds(move)
print 'Auto-update LEDs is', ('enabled' if auto_update_leds else 'disabled')

# Loop and update the controller
while True:
    # Get the latest input report from the controller
    while move.poll(): pass

    # Grab the latest image from the camera
    tracker.update_image()
    # Update all tracked controllers
    tracker.update()

    # Optional and not required by default (see auto_update_leds above)
    #r, g, b = tracker.get_color(move)
    #move.set_leds(r, g, b)
    #move.update_leds()

    # Check the tracking status
    status = tracker.get_status(move)
    if status == psmove.Tracker_TRACKING:
        x, y, radius = tracker.get_position(move)
        print 'Position: (%5.2f, %5.2f), Radius: %3.2f, Trigger: %3d' % (
                x, y, radius, move.get_trigger())
    elif status == psmove.Tracker_CALIBRATED:
        print 'Not currently tracking.'
    elif status == psmove.Tracker_CALIBRATION_ERROR:
        print 'Calibration error.'
    elif status == psmove.Tracker_NOT_CALIBRATED:
        print 'Controller not calibrated.'

