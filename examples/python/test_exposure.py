
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
import time

if psmove.count_connected() < 1:
    print('No controller connected')
    sys.exit(1)

move = psmove.PSMove()

if move.connection_type != psmove.Conn_Bluetooth:
    print('Please connect controller via Bluetooth')
    sys.exit(1)

tracker = psmove.PSMoveTracker()

while tracker.enable(move) != psmove.Tracker_CALIBRATED:
    pass

while True:
    tracker.update_image()
    tracker.update()

    changed = False
    while move.poll():
        pressed, released = move.get_button_events()
        if pressed & psmove.Btn_TRIANGLE:
            tracker.exposure = psmove.Exposure_LOW
            changed = True
        elif pressed & psmove.Btn_CIRCLE:
            tracker.exposure = psmove.Exposure_MEDIUM
            changed = True
        elif pressed & psmove.Btn_CROSS:
            tracker.exposure = psmove.Exposure_HIGH
            changed = True

    if changed:
        tracker.disable(move)
        tracker.dimming = 0
        while tracker.enable(move) != psmove.Tracker_CALIBRATED:
            pass

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


