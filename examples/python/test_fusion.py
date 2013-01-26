
#
# PS Move API - An interface for the PS Move Motion Controller
# Copyright (c) 2013 Thomas Perl <m@thp.io>
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

# This example requires both pyglet and the Python OpenGL bindings to work

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'build'))

import pyglet
from pyglet.gl import *
from OpenGL.GLUT import *
import psmove

glutInit(sys.argv)

window = pyglet.window.Window(width=640, height=480)

tracker = psmove.PSMoveTracker()
tracker.set_mirror(True)

def load_matrix(mode, matrix):
    glMatrixMode(mode)
    glLoadMatrixf((GLfloat*16)(*[matrix.at(i) for i in range(16)]))

fusion = psmove.PSMoveFusion(tracker, 1, 1000)
projection_matrix = fusion.get_projection_matrix()

move = psmove.PSMove()
move.enable_orientation(True)
move.reset_orientation()

while tracker.enable(move) != psmove.Tracker_CALIBRATED:
    pass

glEnable(GL_DEPTH_TEST)
glEnable(GL_BLEND)
glEnable(GL_LIGHTING)
glEnable(GL_LIGHT0)
glEnable(GL_COLOR_MATERIAL)

@window.event
def on_draw():
    while move.poll():
        pressed, released = move.get_button_events()
        if pressed & psmove.Btn_MOVE:
            move.reset_orientation()

    tracker.update_image()
    tracker.update()

    window.clear()

    load_matrix(GL_PROJECTION, projection_matrix)
    load_matrix(GL_MODELVIEW, fusion.get_modelview_matrix(move))

    status = tracker.get_status(move)
    if status == psmove.Tracker_TRACKING:
        glColor4f(1., 1., 1., 1.)
    else:
        glColor4f(1., 0., 0., .5)
    glutSolidCube(1)

pyglet.app.run()

