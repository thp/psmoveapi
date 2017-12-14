
#
# PS Move API - An interface for the PS Move Motion Controller
# Copyright (c) 2011 Thomas Perl <m@thp.io>
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

if move.connection_type == psmove.Conn_Bluetooth:
    print('bluetooth')
elif move.connection_type == psmove.Conn_USB:
    print('usb')
else:
    print('unknown')

if move.connection_type != psmove.Conn_Bluetooth:
    print('Please connect controller via Bluetooth')
    sys.exit(1)

while True:
    # Get the latest input report from the controller
    while move.poll(): pass

    trigger_value = move.get_trigger()
    move.set_leds(trigger_value, 0, 0)
    move.update_leds()

    buttons = move.get_buttons()
    if buttons & psmove.Btn_TRIANGLE:
        print('triangle pressed')
        move.set_rumble(trigger_value)
    else:
        move.set_rumble(0)

    battery = move.get_battery()
    if battery == psmove.Batt_CHARGING:
        print('battery charging via USB')
    elif battery >= psmove.Batt_MIN and battery <= psmove.Batt_MAX:
        print('battery: %d / %d' % (battery, psmove.Batt_MAX))
    else:
        print('unknown battery value:', battery)

    print('accel:', (move.ax, move.ay, move.az))
    print('gyro:', (move.gx, move.gy, move.gz))
    print('magnetometer:', (move.mx, move.my, move.mz))

    time.sleep(.1)

