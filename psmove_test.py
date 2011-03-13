
# PS Move API
# Copyright (c) 2011 Thomas Perl <thp.io/about>
# All Rights Reserved

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build'))

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

