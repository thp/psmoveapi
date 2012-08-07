
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
import random
import psmove

def mapidx(l):
    x, y = l.split(': ')
    return (y, int(x))

# Run mk-index.py to create the file serials.txt, which contains an ordered
# list of controllers' serial numbers (btaddr)
serial_to_idx = dict(list(map(mapidx, open('serials.txt').read().splitlines())))

count = psmove.count_connected()
print('Connected controllers:', count)
print('Serials:', len(serial_to_idx))
assert count == len(serial_to_idx)

moves = [move for idx, move in
        sorted([(serial_to_idx[m.get_serial()], m)
            for m in (psmove.PSMove(i) for i in range(count))])]


for move in moves:
    move.intensity = 0.

i = 0
while True:
    for idx, move in enumerate(moves):
        move.intensity *= random.uniform(.7, .9)
        if (i%len(moves)) == idx:
            move.intensity = 1.
        move.set_leds(0, int(255*move.intensity), int(255*move.intensity))
        move.update_leds()
    i += 1
    time.sleep(.5)

