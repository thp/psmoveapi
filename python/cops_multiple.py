
# PS Move API
# Copyright (c) 2011 Thomas Perl <thp.io/about>
# All Rights Reserved

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

import time
import psmove

if psmove.count_connected() < 2:
    print 'This examples requires at least 2 controllers.'
    sys.exit(1)

a = psmove.PSMove(0)
b = psmove.PSMove(1)

i = 0
while True:
    if i % 12 in (1, 3, 5):
        a.set_leds(255, 0, 0)
    elif i % 12 in (7, 9, 11):
        b.set_leds(0, 0, 255)
    else:
        a.set_leds(0, 0, 0)
        b.set_leds(0, 0, 0)

    a.update_leds()
    b.update_leds()
    time.sleep(.05)
    i += 1

