
# PS Move API
# Copyright (c) 2011 Thomas Perl <thp.io/about>
# All Rights Reserved

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

import time
import psmove

count = psmove.count_connected()
print 'Connected controllers:', count

colors = [
        (255, 0, 0),
        (0, 255, 0),
        (0, 0, 255),
        (255, 255, 0),
        (0, 255, 255),
        (255, 0, 255),
]

for i in range(count):
    move = psmove.PSMove(i)
    move.set_leds(*colors[i%len(colors)])
    move.update_leds()

