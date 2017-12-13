
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

# Test the sensor fusion part of PS Move API with PyGame (tracker + orientation)

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'build'))

import pygame
from pygame.locals import *
from OpenGL.GL import *
from OpenGL.GLUT import *
import psmove

if psmove.count_connected() < 1:
    print('No controller connected')
    sys.exit(1)

glutInit(sys.argv)

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

if move.connection_type != psmove.Conn_Bluetooth:
    print('Please connect controller via Bluetooth')
    sys.exit(1)

while tracker.enable(move) != psmove.Tracker_CALIBRATED:
    pass

class CameraTexture:
    def __init__(self, tracker):
        self.tracker = tracker
        self.id = glGenTextures(1)

        glEnable(GL_TEXTURE_2D)
        glBindTexture(GL_TEXTURE_2D, self.id)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
        glBindTexture(GL_TEXTURE_2D, 0)
        glDisable(GL_TEXTURE_2D)

    def draw(self):
        image = self.tracker.get_image()
        pixels = psmove.cdata(image.data, image.size)

        glDisable(GL_LIGHTING)
        glEnable(GL_TEXTURE_2D)
        glBindTexture(GL_TEXTURE_2D, self.id)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width,
                image.height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                pixels)

        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        glColor4f(1., 1., 1., 1.)
        glBegin(GL_TRIANGLE_STRIP)
        glTexCoord2f(0, 0)
        glVertex3f(-1, 1, 0)
        glTexCoord2f(0, 1)
        glVertex3f(-1, -1, 0)
        glTexCoord2f(1, 0)
        glVertex3f(1, 1, 0)
        glTexCoord2f(1, 1)
        glVertex3f(1, -1, 0)
        glEnd()
        glBindTexture(GL_TEXTURE_2D, 0)
        glDisable(GL_TEXTURE_2D)
        glEnable(GL_LIGHTING)

def on_draw():
    while move.poll():
        pressed, released = move.get_button_events()
        if pressed & psmove.Btn_MOVE:
            move.reset_orientation()

    tracker.update_image()
    tracker.update()

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    camtex.draw()

    # Clear depth buffer, so cam texture doesn't cull model
    glClear(GL_DEPTH_BUFFER_BIT)

    load_matrix(GL_PROJECTION, projection_matrix)
    load_matrix(GL_MODELVIEW, fusion.get_modelview_matrix(move))

    status = tracker.get_status(move)
    if status == psmove.Tracker_TRACKING:
        glColor4f(1., 1., 1., 1.)
    else:
        glColor4f(1., 0., 0., .5)
    glutSolidCube(1)

surface = pygame.display.set_mode((640, 480), OPENGL | DOUBLEBUF)
width, height = surface.get_size()

glEnable(GL_DEPTH_TEST)
glEnable(GL_BLEND)
glEnable(GL_LIGHTING)
glEnable(GL_LIGHT0)
glEnable(GL_COLOR_MATERIAL)

camtex = CameraTexture(tracker)

glViewport(0, 0, width, height)
glClearColor(0.0, 0.0, 0.0, 0.0)

while True:
    on_draw()
    pygame.display.flip()


