
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


# EuroPython 2013: "Whack a Cube" PS Move API Example

import pygame
from pygame.locals import *

from OpenGL.GL import *
from ctypes import c_void_p

import math
import operator
import random

import psmove

def to_glfloat(l):
    return (GLfloat*len(l))(*l)

def read_matrix(m):
    return [m.at(i) for i in range(4*4)]

def vector_distance(a, b):
    return math.sqrt(sum((a[i]-b[i])**2 for i in range(3)))

def transpose_4x4_matrix(m):
    return [m[x] for x in [
        0,  4,  8, 12,
        1,  5,  9, 13,
        2,  6, 10, 14,
        3,  7, 11, 15,
    ]]



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
            print ('== Shader Compile Error ==')
            print (error_message)
            print ('='*30)
            print ('\n'.join('%3d %s' % a for a in enumerate(shader_source.splitlines())))
            print ('='*30)

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
            print ('== Program Link Error ==')
            print (error_message)
            print ('='*30)

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

class CameraBackground:
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
        glUniform1f(self.program.uniform('saturation'), 1.0)
                #move.get_trigger() / 255.)
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4)
        glDisableVertexAttribArray(vtxcoord_loc)

        self.texture.unbind()
        self.vertex_buffer.unbind()
        self.program.unbind()


CUBE_VSH = """
attribute vec4 vtxcoord;
attribute vec3 normal;

uniform mat4 projection;
uniform mat4 modelview;

varying float intensity;
varying vec2 tex;

float lighting(void)
{
    float ambient_factor = 0.4;
    float diffuse_factor = 0.4;

    vec3 lightdir = normalize(vec3(1.0, -1.0, 1.0));
    vec3 n = (vec4(normal, 0.0) * modelview).xyz;
    float diffuse = max(0.0, dot(lightdir, normalize(n)));

    return ambient_factor + diffuse_factor * diffuse;
}

void main(void)
{
    gl_Position = vtxcoord * modelview * projection;
    intensity = lighting();
    tex = vec2(0.5) + 0.5 * vtxcoord.xy;
}
"""

CUBE_FSH = """
uniform vec4 color;

varying float intensity;
varying vec2 tex;

void main(void)
{
    gl_FragColor = vec4(color.rgb * intensity, color.a);
}
"""

def build_geometry(normals, vertices, faces):
    for idx, face in enumerate(faces):
        for index in face:
            yield vertices[index]
            yield normals[idx]

class Cube:
    def __init__(self):
        self.program = ShaderProgram(CUBE_VSH, CUBE_FSH)
        self.vertex_buffer = VertexBuffer()

        normals = [
            (0., 1., 0.), (0. , 0., 1.), (1. ,0., 0.),
            (0., 0., -1.), (-1., 0., 0.), (0., -1., 0.),
        ]

        vertices = [
            (1., 1., -1.), (-1., 1., -1.), (1., 1., 1.), (-1., 1., 1.),
            (-1., -1., 1.), (1., -1., 1.), (1., -1., -1.), (-1., -1., -1.),
        ]

        faces = [
            [0, 1, 2, 3], [3, 4, 2, 5], [2, 5, 0, 6],
            [0, 6, 1, 7], [1, 7, 3, 4], [4, 7, 5, 6],
        ]

        vertex_data = [x for v in build_geometry(normals, vertices, faces) for x in v]
        self.count = len(vertex_data) / (3 + 3)

        self.vertex_buffer.bind()
        self.vertex_buffer.data(vertex_data)
        self.vertex_buffer.unbind()

    def draw(self, modelview_matrix, color):
        self.program.bind()
        self.vertex_buffer.bind()

        # Size (in bytes) of GLfloat (32-bit floating point)
        sizeof_float = 4

        # Offset between two vertices in vertex buffer
        stride = (3 + 3) * sizeof_float

        # Offset (in a single vertex) of normal vector
        offset = 3 * sizeof_float

        vtxcoord_loc = self.program.attrib('vtxcoord')
        glEnableVertexAttribArray(vtxcoord_loc)
        glVertexAttribPointer(vtxcoord_loc, 3, GL_FLOAT, GL_FALSE, stride, c_void_p(0))

        normal_loc = self.program.attrib('normal')
        glEnableVertexAttribArray(normal_loc)
        glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, stride, c_void_p(offset))

        glUniformMatrix4fv(self.program.uniform('projection'), 1, GL_TRUE,
                to_glfloat(projection_matrix))
        glUniformMatrix4fv(self.program.uniform('modelview'), 1, GL_TRUE,
                to_glfloat(modelview_matrix))
        glUniform4f(self.program.uniform('color'), *color)

        glDrawArrays(GL_TRIANGLE_STRIP, 0, self.count)

        glDisableVertexAttribArray(normal_loc)
        glDisableVertexAttribArray(vtxcoord_loc)

        self.vertex_buffer.unbind()
        self.program.unbind()


if psmove.count_connected() < 1:
    print('No controller connected')
    sys.exit(1)

tracker = psmove.PSMoveTracker()
tracker.set_mirror(True)

near_plane = 1.0
far_plane = 100.0
fusion = psmove.PSMoveFusion(tracker, near_plane, far_plane)
projection_matrix = read_matrix(fusion.get_projection_matrix())

move = psmove.PSMove()
move.enable_orientation(True)
move.reset_orientation()

if move.connection_type != psmove.Conn_Bluetooth:
    print('Please connect controller via Bluetooth')
    sys.exit(1)

pygame.init()
if '-f' not in sys.argv:
    surface = pygame.display.set_mode((640, 480), OPENGL | DOUBLEBUF)
else:
    surface = pygame.display.set_mode((0, 0), OPENGL | DOUBLEBUF | FULLSCREEN)
width, height = surface.get_size()

glEnable(GL_DEPTH_TEST)
glEnable(GL_BLEND)
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

camera_background = CameraBackground(tracker)
cube = Cube()

glViewport(0, 0, width, height)
glClearColor(0.0, 0.0, 0.0, 0.0)

while tracker.enable(move) != psmove.Tracker_CALIBRATED:
    pass

class Constants:
    # Number of milliseconds between each tick
    TICK_MS = 20

    # Maximum simultaneous highlighted buttons
    MAX_SIMULTANEOUS_HIGHLIGHTED = 6

    # Minimum and maximum number of required hits
    REQUIRED_HITS_RANGE = (1, 2)

    # Minimum and maximum ticks between highlight events
    HIGHLIGHT_TICKS_RANGE = (30, 100)

    # Minimum and maximum ticks before a highlight times out
    TIMEOUT_TICKS_RANGE = (500, 1000)

    # How long a green cube stays lit after hitting to zero
    AFTERGLOW_TICKS = 100

    # Number of columns and rows in button grid (columns, rows)
    BUTTON_GRID_SIZE = (3, 3)

    # Distance between button centers
    BUTTON_DISTANCE = 4.0

    # Minimum ticks between two consecutive hits
    MIN_TICKS_BETWEEN_HITS = 25

    # "Hit" distance threshold from controller to button
    COLLISION_DISTANCE_TRESHOLD = 3.0


class Button:
    def __init__(self, index):
        rows, columns = Constants.BUTTON_GRID_SIZE
        self.gridpos = int(index % columns), int(index / columns)

        # Position of button in 3d space (based on grid position)
        x = (self.gridpos[0] - int(columns / 2))
        y = self.gridpos[1] * 0.4 - 1.2
        z = -(3.0 + self.gridpos[1])
        self.pos = tuple(v * Constants.BUTTON_DISTANCE for v in (x, y, z))

        self.highlight_value = 0
        self.hit_block_ticks = 0
        self.required_hits = 0
        self.hits = 0

    def render(self):
        matrix = transpose_4x4_matrix([
            1.0, 0.0, 0.0, self.pos[0],
            0.0, 1.0, 0.0, self.pos[1],
            0.0, 0.0, 1.0, self.pos[2],
            0.0, 0.0, 0.0, 1.0,
        ])

        color = (1.0, 1.0, 1.0, 0.2)

        if self.is_highlighted():
            if self.hits == self.required_hits:
                color = (0.0, 1.0, 0.0, 1.0) # green
            elif self.hits > 0:
                color = (1.0, 0.6, 0.0, 1.0) # orange
            else:
                color = (1.0, 0.0, 0.0, 1.0) # red

        cube.draw(matrix, color)

    def calc_distance(self):
        current_pos = fusion.get_position(move)
        distance = vector_distance(current_pos, self.pos)
        if distance < Constants.COLLISION_DISTANCE_TRESHOLD:
            self.hit_by_controller()

    def hit_by_controller(self):
        if self.hit_block_ticks > 0:
            # blocking this hit - minimum time between hits not met
            return

        self.hit_block_ticks = Constants.MIN_TICKS_BETWEEN_HITS

        # Increase hit counter
        if self.hits < self.required_hits:
            self.hits += 1
            if self.hits == self.required_hits:
                print ('+1 SCORE')
                self.highlight_value = Constants.AFTERGLOW_TICKS

    def highlight_now(self):
        self.highlight_value = random.randint(*Constants.TIMEOUT_TICKS_RANGE)
        self.required_hits = random.randint(*Constants.REQUIRED_HITS_RANGE)
        self.hits = 0

    def is_highlighted(self):
        return self.highlight_value > 0

    def tick(self):
        if self.highlight_value > 0:
            self.highlight_value -= 1

        if self.hit_block_ticks > 0:
            self.hit_block_ticks -= 1


class Highlighter:
    def __init__(self, buttons):
        self.buttons = buttons
        self.ticks_until_next_highlight = 0

    def highlight_random_button(self):
        available_buttons = [button for button in self.buttons
                if not button.is_highlighted()]

        highlighted_count = len(self.buttons) - len(available_buttons)
        if highlighted_count >= Constants.MAX_SIMULTANEOUS_HIGHLIGHTED:
            # Too many buttons already highlighted - skip this time
            return

        chosen = random.choice(available_buttons)
        chosen.highlight_now()

    def tick(self):
        if self.ticks_until_next_highlight > 0:
            # Not quite there yet
            self.ticks_until_next_highlight -= 1
            return

        self.highlight_random_button()

        # Schedule next highlight run
        ticks = random.randint(*Constants.HIGHLIGHT_TICKS_RANGE)
        self.ticks_until_next_highlight = ticks

buttons = [Button(i) for i in range(operator.mul(*Constants.BUTTON_GRID_SIZE))]

highlighter = Highlighter(buttons)

tick_receivers = [highlighter] + buttons

class TickEmitter:
    def __init__(self, tick_ms, receivers):
        self.tick_ms = tick_ms
        self.receivers = receivers
        self.time_accumulator = 0
        self.last_time = pygame.time.get_ticks()

    def update(self):
        now_time = pygame.time.get_ticks()
        diff = (now_time - self.last_time)
        self.time_accumulator += diff
        self.last_time = now_time

        while self.time_accumulator > self.tick_ms:
            for receiver in self.receivers:
                receiver.tick()
            self.time_accumulator -= self.tick_ms

tick_emitter = TickEmitter(Constants.TICK_MS, tick_receivers)

while True:
    while move.poll():
        pressed, released = move.get_button_events()
        if pressed & psmove.Btn_MOVE:
            move.reset_orientation()
        elif pressed & psmove.Btn_PS:
            sys.exit(0)

    tracker.update_image()
    tracker.update()

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    camera_background.draw()

    # Clear depth buffer, so cam texture doesn't cull model
    glClear(GL_DEPTH_BUFFER_BIT)

    status = tracker.get_status(move)
    if status == psmove.Tracker_TRACKING:
        color = (1.0, 1.0, 1.0, 1.0)
    else:
        color = (1.0, 0.0, 0.0, 0.5)

    modelview_matrix = read_matrix(fusion.get_modelview_matrix(move))
    cube.draw(modelview_matrix, color)

    for button in buttons:
        button.calc_distance()
        button.render()

    tick_emitter.update()

    pygame.display.flip()

