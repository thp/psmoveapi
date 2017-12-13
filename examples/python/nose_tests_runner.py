
#
# PS Move API - An interface for the PS Move Motion Controller
# Copyright (c) 2012 Thomas Perl <m@thp.io>
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
Ambient display for realtime "nosetests" monitoring

This utility uses the "nosetests" utility to check if the Python unit
tests in the directory it is run in are running successfully or not.

In case of a successful run (no errors), the controller turns green,
in case of a test failure (or error), the controller turns red and the
error output is shown on standard output.
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'build'))

import time
import psmove

import threading
import subprocess

class Visualizer:
    def __init__(self):
        self._move = psmove.PSMove()

        self._fade_to = (0, 0, 0)
        self._color = (0, 0, 0)

        self._thread = threading.Thread(target=self._fade_thread_proc)
        self._thread.setDaemon(True)
        self._thread.start()

    def _update_leds(self):
        self._move.set_leds(*map(int, self._color))
        self._move.update_leds()

    def mix(self, a, b, f):
        return tuple(a*f + b*(1-f) for a, b in zip(a, b))

    def _fade_thread_proc(self):
        while True:
            while self._color != self._fade_to:
                self._color = self.mix(self._fade_to, self._color, .05)
                self._update_leds()
                time.sleep(.01)
            while self._color == self._fade_to:
                time.sleep(1)
                self._update_leds()

    def fade_to(self, r, g, b):
        self._fade_to = (r, g, b)


class Signal:
    def __init__(self):
        self._connections = []

    def connect(self, f):
        self._connections.append(f)

    def emit(self, *args, **kwargs):
        for f in self._connections:
            try:
                f(*args, **kwargs)
            except Exception, e:
                print e

class TestRunner:
    COMMAND = 'nosetests'
    RED, GREEN = range(2)

    def __init__(self):
        self.new_color = Signal()
        self.error_output = Signal()

    def run_tests(self):
        process = subprocess.Popen([TestRunner.COMMAND],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        result = process.wait()
        if result == 0:
            self.new_color.emit(TestRunner.GREEN)
            self.error_output.emit('\n'.join((stdout, stderr)).strip())
        else:
            self.error_output.emit('\n'.join((stdout, stderr)).strip())
            self.new_color.emit(TestRunner.RED)


class Glue:
    def __init__(self, visualizer, test_runner):
        self.visualizer = visualizer
        self.test_runner = test_runner

        self.test_runner.new_color.connect(self.on_new_color)
        self.test_runner.error_output.connect(self.on_output)

    def on_output(self, message):
        os.system('clear')
        print message

    def on_new_color(self, color):
        if color == TestRunner.RED:
            self.visualizer.fade_to(255, 0, 0)
        elif color == TestRunner.GREEN:
            self.visualizer.fade_to(0, 255, 0)

    def run(self):
        while True:
            self.test_runner.run_tests()
            time.sleep(2)

if __name__ == '__main__':
    if psmove.count_connected() < 1:
        print('No controller connected')
        sys.exit(1)

    visualizer = Visualizer()
    test_runner = TestRunner()
    glue = Glue(visualizer, test_runner)
    glue.run()

