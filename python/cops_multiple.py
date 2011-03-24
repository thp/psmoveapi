
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

