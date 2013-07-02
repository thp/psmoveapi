
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

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'build'))


# EuroPython 2013: "cubes" PS Move API Example

import pygame
from pygame.locals import *

from OpenGL.GL import *
from OpenGL.GLUT import *

import psmove

glutInit(sys.argv)

tracker = psmove.PSMoveTracker()
tracker.set_mirror(True)

def matrix_as_float_array(matrix):
    mdata = [matrix.at(i) for i in range(4*4)]
    return (GLfloat*16)(*mdata)

def load_matrix(mode, matrix):
    glMatrixMode(mode)
    glLoadMatrixf(matrix_as_float_array(matrix))

near_plane = 1.0
far_plane = 100.0
fusion = psmove.PSMoveFusion(tracker, near_plane, far_plane)
projection_matrix = fusion.get_projection_matrix()

move = psmove.PSMove()
move.enable_orientation(True)
move.reset_orientation()


class Texture:
    def __init__(self):
        self.id = glGenTextures(1)

        self.bind()
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
        self.unbind()

    def __del__(self):
        glDeleteTextures([self.id])

    def bind(self):
        glEnable(GL_TEXTURE_2D)
        glBindTexture(GL_TEXTURE_2D, self.id)

    def unbind(self):
        glBindTexture(GL_TEXTURE_2D, 0)
        glDisable(GL_TEXTURE_2D)

    def load_rgb(self, width, height, pixels):
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                0, GL_RGB, GL_UNSIGNED_BYTE, pixels)


class VertexBuffer:
    def __init__(self):
        self.id = glGenBuffers(1)

    def __del__(self):
        glDeleteBuffers([self.id])

    def bind(self):
        glBindBuffer(GL_ARRAY_BUFFER, self.id)

    def unbind(self):
        glBindBuffer(GL_ARRAY_BUFFER, 0)

    def data(self, data):
        glBufferData(GL_ARRAY_BUFFER, (GLfloat*len(data))(*data), GL_STREAM_DRAW)


class Shader:
    def __init__(self, shader_type, shader_source):
        self.id = glCreateShader(shader_type)
        glShaderSource(self.id, [shader_source])
        glCompileShader(self.id)

        error_message = glGetShaderInfoLog(self.id)
        if error_message:
            print '== Shader Compile Error =='
            print error_message
            print '='*30
            print '\n'.join('%3d %s' % a for a in enumerate(shader_source.splitlines()))
            print '='*30

    def __del__(self):
        glDeleteShader(self.id)

class ShaderProgram:
    def __init__(self, vertex_shader_src, fragment_shader_src):
        self.id = glCreateProgram()

        # Compile shaders
        self.vertex_shader = Shader(GL_VERTEX_SHADER, vertex_shader_src)
        self.fragment_shader = Shader(GL_FRAGMENT_SHADER, fragment_shader_src)

        # Attach and link shaders
        glAttachShader(self.id, self.vertex_shader.id)
        glAttachShader(self.id, self.fragment_shader.id)
        glLinkProgram(self.id)
        error_message = glGetProgramInfoLog(self.id)
        if error_message:
            print '== Program Link Error =='
            print error_message
            print '='*30

    def bind(self):
        glUseProgram(self.id)

    def unbind(self):
        glUseProgram(0)

    def attrib(self, name):
        return glGetAttribLocation(self.id, name)

    def uniform(self, name):
        return glGetUniformLocation(self.id, name)

    def __del__(self):
        glDeleteProgram(self.id)

CAMERA_VSH = """
attribute vec4 vtxcoord;

varying vec2 tex;

void main(void)
{
    gl_Position = vtxcoord;
    tex = vec2(0.5, 0.5) + (vtxcoord.xy * 0.5);
    tex.y = 1.0 - tex.y;
}
"""

CAMERA_FSH = """
varying vec2 tex;

uniform sampler2D texture;

uniform float saturation;

void main(void)
{
    vec3 color = texture2D(texture, tex).rgb;
    vec3 grey = vec3(length(color));
    gl_FragColor = vec4(mix(grey, color, saturation), 1.0);
}
"""

class CameraTexture:
    def __init__(self, tracker):
        self.tracker = tracker
        self.texture = Texture()
        self.program = ShaderProgram(CAMERA_VSH, CAMERA_FSH)
        self.vertex_buffer = VertexBuffer()
        self.vertex_buffer.bind()
        self.vertex_buffer.data([
            -1.0, -1.0,
            -1.0, +1.0,
            +1.0, -1.0,
            +1.0, +1.0,
        ])
        self.vertex_buffer.unbind()

    def draw(self):
        image = self.tracker.get_image()
        pixels = psmove.cdata(image.data, image.size)

        self.program.bind()
        self.vertex_buffer.bind()
        self.texture.bind()
        self.texture.load_rgb(image.width, image.height, pixels)

        vtxcoord_loc = self.program.attrib('vtxcoord')
        glEnableVertexAttribArray(vtxcoord_loc)
        glVertexAttribPointer(vtxcoord_loc, 2, GL_FLOAT, GL_FALSE, 0, None)
        glUniform1f(self.program.uniform('saturation'),
                move.get_trigger() / 255.)
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4)
        glDisableVertexAttribArray(vtxcoord_loc)

        self.texture.unbind()
        self.vertex_buffer.unbind()
        self.program.unbind()

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

while tracker.enable(move) != psmove.Tracker_CALIBRATED:
    pass

while True:
    on_draw()
    pygame.display.flip()


