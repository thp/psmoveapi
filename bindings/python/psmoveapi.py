#
# PS Move API - An interface for the PS Move Motion Controller
# Copyright (c) 2016 Thomas Perl <m@thp.io>
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


from ctypes import *
import sys
import os
import math

library_path = os.environ.get('PSMOVEAPI_LIBRARY_PATH', None)
if library_path:
    library_prefix = library_path + os.sep
else:
    library_prefix = ''

if os.name == 'nt':
    libpsmoveapi = CDLL(library_prefix + 'psmoveapi.dll')
elif sys.platform == 'darwin':
    libpsmoveapi = CDLL(library_prefix + 'libpsmoveapi.dylib')
else:
    libpsmoveapi = CDLL(library_prefix + 'libpsmoveapi.so')


class RGB(Structure):
    _fields_ = [
        ('r', c_float),
        ('g', c_float),
        ('b', c_float),
    ]

    def __repr__(self):
        return '<%s (%.2f, %.2f, %.2f)>' % (type(self).__name__, self.r, self.g, self.b)

    def __mul__(self, factor):
        return RGB(self.r * factor, self.g * factor, self.b * factor)

    def __add__(self, other):
        return RGB(self.r + other.r, self.g + other.g, self.b + other.b)

    def __truediv__(self, factor):
        return RGB(self.r / factor, self.g / factor, self.b / factor)


class Vec3(Structure):
    _fields_ = [
        ('x', c_float),
        ('y', c_float),
        ('z', c_float),
    ]

    def __repr__(self):
        return '<%s (%.2f, %.2f, %.2f)>' % (type(self).__name__, self.x, self.y, self.z)

    def length(self):
        return math.sqrt(self.x * self.x + self.y * self.y + self.z * self.z)

class ControllerStruct(Structure):
    _fields_ = [
        ('index', c_int),
        ('serial', c_char_p),
        ('bluetooth', c_int),
        ('usb', c_int),

        ('color', RGB),
        ('rumble', c_float),

        ('buttons', c_int),
        ('pressed', c_int),
        ('released', c_int),
        ('trigger', c_float),
        ('accelerometer', Vec3),
        ('gyroscope', Vec3),
        ('magnetometer', Vec3),
        ('battery', c_int),

        ('user_data', c_void_p),
    ]

class Button(object):
    TRIANGLE = 1 << 4
    CIRCLE = 1 << 5
    CROSS = 1 << 6
    SQUARE = 1 << 7

    SELECT = 1 << 8
    START = 1 << 11

    PS = 1 << 16
    MOVE = 1 << 19
    T = 1 << 20

    VALUES = [TRIANGLE, CIRCLE, CROSS, SQUARE, SELECT, START, PS, MOVE, T]

ControllerFunc = CFUNCTYPE(None, POINTER(ControllerStruct), c_void_p)

class EventReceiver(Structure):
    _fields_ = [
        ('connect', ControllerFunc),
        ('update', ControllerFunc),
        ('disconnect', ControllerFunc),
    ]

libpsmoveapi.psmoveapi_init.argtypes = [POINTER(EventReceiver), c_void_p]

class Controller(object):
    def __init__(self, controller):
        self._controller = controller

    def __repr__(self):
        return '<Controller #{} {}>'.format(self.index, self.serial)

    def now_pressed(self, button):
        return (self._controller.pressed & button) != 0

    def still_pressed(self, button):
        return (self._controller.buttons & button) != 0

    def now_released(self, button):
        return (self._controller.released & button) != 0

    def __setattr__(self, name, value):
        if name != '_controller' and hasattr(self._controller, name):
            setattr(self._controller, name, value)

        return super().__setattr__(name, value)

    def __getattr__(self, name):
        if name.startswith('_'):
            return super().__getattr__(name)
        elif name == 'serial':
            return self._controller.serial.decode('utf-8')
        elif name in ('bluetooth', 'usb'):
            return bool(getattr(self._controller, name))
        elif hasattr(self._controller, name):
            return getattr(self._controller, name)

        return super().__getattr__(name)


class PSMoveAPI(object):
    _instances = 0

    def __init__(self):
        self._inited = False
        self._controllers = {}

        if PSMoveAPI._instances > 0:
            raise RuntimeError('Only once instance of PSMoveAPI can exist')

        PSMoveAPI._instances += 1
        self.recv = EventReceiver()
        self.recv.connect = ControllerFunc(self._on_connect)
        self.recv.update = ControllerFunc(self._on_update)
        self.recv.disconnect = ControllerFunc(self._on_disconnect)
        libpsmoveapi.psmoveapi_init(self.recv, None)
        self._inited = True

    def update(self):
        libpsmoveapi.psmoveapi_update()

    def _on_connect(self, controller, user_data):
        self._controllers[controller[0].serial] = Controller(controller[0])
        self.on_connect(self._controllers[controller[0].serial])

    def _on_update(self, controller, user_data):
        self.on_update(self._controllers[controller[0].serial])

    def _on_disconnect(self, controller, user_data):
        self.on_disconnect(self._controllers[controller[0].serial])
        del self._controllers[controller[0].serial]

    def __del__(self):
        if self._inited:
            libpsmoveapi.psmoveapi_quit()
        PSMoveAPI._instances -= 1
