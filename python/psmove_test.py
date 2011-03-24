
#
# PS Move API - An interface for the PS Move Motion Controller
# Copyright (C) 2011 Thomas Perl <m@thp.io>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

import psmove

move = psmove.PSMove()

if move.connection_type == psmove.Conn_Bluetooth:
    print 'bluetooth'
elif move.connection_type == psmove.Conn_USB:
    print 'usb'
else:
    print 'unknown'

while True:
    if move.poll():
        trigger_value = move.get_trigger()
        move.set_leds(trigger_value, 0, 0)
        move.update_leds()
        buttons = move.get_buttons()
        if buttons & psmove.Btn_TRIANGLE:
            print 'triangle pressed'
            move.set_rumble(trigger_value)
        else:
            move.set_rumble(0)
        print 'accel:', (move.ax, move.ay, move.az)
        print 'gyro:', (move.gx, move.gy, move.gz)
        print 'magnetometer:', (move.mx, move.my, move.mz)

