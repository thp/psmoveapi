
# PS Move API
# Copyright (c) 2011 Thomas Perl <thp.io/about>
# All Rights Reserved

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

import time
import psmove

move = psmove.PSMove()

i = 0
while True:
    if i % 12 in (1, 3, 5):
        move.set_leds(255, 0, 0)
    elif i % 12 in (7, 9, 11):
        move.set_leds(0, 0, 255)
    else:
        move.set_leds(0, 0, 0)

    if i % 20 > 10:
        move.set_rumble(120)
    else:
        move.set_rumble(0)
    move.update_leds()
    time.sleep(.05)
    i += 1

