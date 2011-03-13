
# PS Move API
# Copyright (c) 2011 Thomas Perl <thp.io/about>
# All Rights Reserved

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

import time
import math
import psmove

move = psmove.PSMove()

i = 0
while True:
    r = int(128+128*math.sin(i))
    move.set_leds(r, 255-r, 0)
    move.update_leds()
    time.sleep(.1)
    i += .2

