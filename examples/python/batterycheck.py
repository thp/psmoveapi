
#
# PS Move API - An interface for the PS Move Motion Controller
# Copyright (c) 2011, 2012 Thomas Perl <m@thp.io>
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

import time
import psmove


while True:
    print('checking...')
    psmove.reinit() # need this to re-enumerate new/disconnected devices
    moves = [psmove.PSMove(x) for x in range(psmove.count_connected())]

    print('connections:', sum(m.connection_type == psmove.Conn_USB
            for m in moves), 'usb,', \
                    sum(m.connection_type == psmove.Conn_Bluetooth
            for m in moves), 'bluetooth.')

    for move in moves:
        if move.connection_type == psmove.Conn_Bluetooth:
            # wait for the first valid input report from that controller
            while not move.poll():
                pass

            battery = move.get_battery()

            if battery == 5: # 100% - green
                move.set_leds(0, 255, 0)
            elif battery == 4: # 80% - green-ish
                move.set_leds(128, 200, 0)
            elif battery == 3: # 60% - yellow
                move.set_leds(255, 255, 0)
            elif battery > 5: # charging
                move.set_leds(0, 0, 255)
            else: # <= 40% - red
                move.set_leds(255, 0, 0)
            move.update_leds()
        del move # make sure the controller object is freed to avoid segfaults
    moves = []
    time.sleep(1)

