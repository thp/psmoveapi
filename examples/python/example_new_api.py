import os
import sys
import time

BASE = os.path.join(os.path.dirname(__file__), '..', '..')

if 'PSMOVEAPI_LIBRARY_PATH' not in os.environ:
    os.environ['PSMOVEAPI_LIBRARY_PATH'] = os.path.join(BASE, 'build')

sys.path.insert(0, os.path.join(BASE, 'bindings', 'python'))

import psmoveapi


class RedTrigger(psmoveapi.PSMoveAPI):
    def __init__(self):
        super().__init__()
        self.quit = False

    def on_connect(self, controller):
        controller.connection_time = time.time()
        print('Controller connected:', controller, controller.connection_time)

    def on_update(self, controller):
        print('Update controller:', controller, int(time.time() - controller.connection_time))
        print(controller.accelerometer, '->', controller.color, 'usb:', controller.usb, 'bt:', controller.bluetooth)
        up_pointing = min(1, max(0, 0.5 + 0.5 * controller.accelerometer.y))
        controller.color = psmoveapi.RGB(controller.trigger, up_pointing, 1.0 if controller.usb else 0.0)
        if controller.now_pressed(psmoveapi.Button.PS):
            self.quit = True

    def on_disconnect(self, controller):
        print('Controller disconnected:', controller)


api = RedTrigger()
while not api.quit:
    api.update()

