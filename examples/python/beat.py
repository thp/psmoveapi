
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

"""
Pulsating light demo

Press the Move button at the first beat, then press it again after 4 beats.
Watch the sphere glow up to the beat. Keep SQUARE pressed to let it glow up
every 2 beats. Keep TRIANGLE pressed to let it glow up every beat. Press the
Move button to reset, then start again (first beat, 4th beat, ...).
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'build'))

import time
import math
import psmove

if psmove.count_connected() < 1:
    print('No controller connected')
    sys.exit(1)

move = psmove.PSMove()
move.set_rate_limiting(1)

if move.connection_type != psmove.Conn_Bluetooth:
    print('Please connect controller via Bluetooth')
    sys.exit(1)

current_beat = 0
old_buttons = 0
last_blink = 0
intensity = 0
divisor = 1
last_decrease = 0

while True:
    while move.poll():
        buttons = move.get_buttons()
        if buttons & psmove.Btn_MOVE and not old_buttons & psmove.Btn_MOVE:
            print time.time(), 'press'

            if current_beat == 0:
                print 'init'
                current_beat = time.time()
            elif current_beat < 10000:
                print 'reset'
                current_beat = 0
            else:
                print 'run!'
                current_beat = time.time() - current_beat
                last_blink = time.time()

        if buttons & psmove.Btn_TRIANGLE:
            divisor = 4
        elif buttons & psmove.Btn_SQUARE:
            divisor = 2
        else:
            divisor = 1
        old_buttons = buttons

    intensity *= .9999

    if current_beat > 0 and current_beat < 10000:
        if last_blink == 0 or last_blink + (current_beat/divisor) < time.time():
            last_blink += current_beat/divisor
            print current_beat, 'blink'
            intensity = 255.

    move.set_leds(*map(int, [intensity]*3))
    move.update_leds()

