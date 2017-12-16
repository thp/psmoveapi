
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

import psmove
import math

if psmove.count_connected() < 1:
    print('No controller connected')
    sys.exit(1)

move = psmove.PSMove()

if move.connection_type != psmove.Conn_Bluetooth:
    print('Please connect controller via Bluetooth')
    sys.exit(1)

readings = []
lastmeans = []
NUM_READINGS = 50
NUM_MEANS = 10

def mean(readings):
    sums = list(map(sum, list(zip(*readings))))
    return [s/len(readings) for s in sums]

def calc_diffsums(lastmeans, currentmeans):
    cx, cy, cz = currentmeans
    x, y, z = 0, 0, 0
    for lx, ly, lz in lastmeans:
        x += (cx-lx)
        y += (cy-ly)
        z += (cz-lz)

    return x, y, z

def stdev(readings):
    def calc_stdev(l):
        print(l)
        mean = sum(l) / len(l)
        x2 = [x**2 for x in l]
        print(x2)
        print(sum(x2), len(l), mean*mean)
        return math.sqrt(sum(x2)/len(l) - mean*mean)
    return list(map(calc_stdev, readings))

while True:
    if move.poll():
        reading = (move.mx, move.my, move.mz)
        readings.append(reading)
        if len(readings) > NUM_READINGS:
            readings.pop(0)

        lastmeans.append(mean(readings))
        if len(lastmeans) > NUM_MEANS:
            lastmeans.pop(0)

        rx, ry, rz = list(zip(*readings))
        l = [abs(x) for x in (move.mx, move.my, move.mz) if x != 0]
        length = math.sqrt(move.mx**2 + move.my**2 + move.mz**2)

        print(('%10.3f '*3) % reading, length)
        diffsums = calc_diffsums(lastmeans, mean(readings))
        change = sum(map(abs, diffsums))

        cutoff = 400
        if length > cutoff:
            factor = (length-cutoff) / (2048 - cutoff)
            if factor > .5:
                move.set_leds(255, 0, 0)
            else:
                move.set_leds(0, 0, 0)

        if len(l) == 3:
            proximity = math.pow(float(sum(l))/len(l)/2048, 3)

        move.update_leds()
