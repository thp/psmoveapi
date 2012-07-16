
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


from __future__ import division

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

import psmove

from PySide.QtCore import *
from PySide.QtGui import *
import threading

move = psmove.PSMove()

def gyro_frames(move):
    while True:
        if move.poll():
            yield move.get_half_frame(psmove.Sensor_Gyroscope,
                    psmove.Frame_FirstHalf)
            yield move.get_half_frame(psmove.Sensor_Gyroscope,
                    psmove.Frame_SecondHalf)


class DriftCalculator:
    MINIMUM_READINGS = 500

    def __init__(self):
        self.values = [0, 0, 0]
        self.frames = 0
        self.done = False
        self.bias = [0, 0, 0]

    def feed(self, values):
        self.values = map(sum, zip(self.values, values))
        self.frames += 1
        if self.frames == self.MINIMUM_READINGS:
            self.bias = map(lambda x: x / self.frames, self.values)
            self.done = True

    def fix(self, values):
        return map(lambda (a, b): a - b, zip(values, self.bias))

class SumHandler:
    def __init__(self):
        self.values = [0, 0, 0]
        self.last_values = [0, 0, 0]

    def reset(self):
        self.values = [0, 0, 0]
        self.last_values = [0, 0, 0]

    def add(self, values):
        self.values = map(sum, zip(self.values, values))

    def diff(self):
        last_values = self.last_values
        self.last_values = self.values
        return map(lambda (a, b): a - b, zip(self.values, last_values))


class Scaler:
    def __init__(self):
        self.factors = [672696,]*3

    def scale(self, values):
        return map(lambda (a, b): a / b, zip(values, self.factors))

class Event:
    UP, DOWN = range(2)

    def __init__(self, type, button):
        self.type = type
        self.button = button

class Buttons:
    def __init__(self, move):
        self.move = move
        self.last_buttons = 0

    def events(self):
        buttons = self.move.get_buttons()
        diff = buttons ^ self.last_buttons
        value = 1
        while diff > 0:
            if value & diff:
                yield Event(Event.DOWN if value & buttons else Event.UP, value)
                diff -= value
            value <<= 1
        self.last_buttons = buttons


calculator = DriftCalculator()
summer = SumHandler()
buttons = Buttons(move)
scaler = Scaler()

button_events = buttons.events()

class MyThread(QThread):
    def __init__(self):
        QThread.__init__(self)

    def run(self):
        for idx, frame in enumerate(gyro_frames(move)):
            if idx % 100 == 0:
                if not calculator.done:
                    print '.'*int(idx / 100)

            if calculator.done:
                for event in buttons.events():
                    if event.type == Event.UP:
                        #print 'UP:', event.button
                        pass
                    elif event.type == Event.DOWN:
                        #print 'DOWN:', event.button
                        if event.button == psmove.Btn_CROSS:
                            print '-->', summer.diff()
                        elif event.button == psmove.Btn_CIRCLE:
                            summer.reset()

                summer.add(calculator.fix(frame))

                #print ' '.join(map(lambda x: '%10.2f' % x, scaler.scale(summer.values)))
                for idx, rotation in enumerate(scaler.scale(summer.values)):
                    transforms[idx].setAngle((-1 if idx != 1 else 1)*rotation*360)
                #rot = scaler.scale(summer.values)[2]
                #rect.setRotation(-rot*360)
                #rect.setRotation(rect.rotation() + 1)
                #print rect.rotation()
                scene.invalidate()
                #self.sleep(.1)
            else:
                calculator.feed(frame)

app = QApplication([])


view = QGraphicsView()
scene = QGraphicsScene(-600, -200, 1200, 400)
rect = scene.addRect(-100, -100, 200, 200, QPen(), Qt.green)
transforms = [QGraphicsRotation(), QGraphicsRotation(), QGraphicsRotation()]
transforms[0].setAxis(Qt.XAxis)
transforms[1].setAxis(Qt.YAxis)
transforms[2].setAxis(Qt.ZAxis)

rect.setTransformations(transforms)
rect.setTransformOriginPoint(rect.rect().center())
view.setScene(scene)
view.show()

thread = MyThread()
thread.start()

app.exec_()

