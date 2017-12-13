
#
# PS Move API - An interface for the PS Move Motion Controller
# Copyright (c) 2011-2012 Thomas Perl <m@thp.io>
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
import math
import psmove

moves = [psmove.PSMove(x) for x in range(psmove.count_connected())]

if not moves:
    print('No controller connected');
    sys.exit(1)

colors = [
   (255, 0, 0),
   (0, 255, 0),
   (0, 0, 255),
   (0, 255, 255),
   (255, 0, 255),
   (255, 255, 0),
]

brightness = 1.
wait_time = 3.
current = 0

def mix(a, b):
    def mix_component(idx):
        return int(brightness*(a[idx]*(1.-x) + b[idx]*float(x)))
    return map(mix_component, range(3))

x = 0.
while True:
    for move in moves:
        move.set_leds(*mix(colors[current], colors[(current+1)%len(colors)]))
        move.update_leds()
    time.sleep(.01)
    x += .001
    if x >= 1.:
        x -= 1.
        current += 1
        current %= len(colors)

