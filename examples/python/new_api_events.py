import os
import sys

BASE = os.path.join(os.path.dirname(__file__), '..', '..')

if 'PSMOVEAPI_LIBRARY_PATH' not in os.environ:
    os.environ['PSMOVEAPI_LIBRARY_PATH'] = os.path.join(BASE, 'build')

sys.path.insert(0, os.path.join(BASE, 'bindings', 'python'))

import psmoveapi


class ButtonEvents(psmoveapi.PSMoveAPI):
    def __init__(self):
        super().__init__()
        self.quit = False

    def on_connect(self, controller):
        ...

    def on_update(self, controller):
        if controller.now_pressed(psmoveapi.Button.CROSS):
            print('Cross was pressed on', controller.serial)
        elif controller.still_pressed(psmoveapi.Button.CROSS):
            print('Cross is still held on', controller.serial)
        elif controller.now_released(psmoveapi.Button.CROSS):
            print('Cross was released on', controller.serial)

        if controller.now_pressed(psmoveapi.Button.PS):
            self.quit = True

    def on_disconnect(self, controller):
        ...


api = ButtonEvents()
while not api.quit:
    api.update()
