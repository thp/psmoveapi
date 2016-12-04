
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 **/

#include <stdio.h>

#include <time.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <list>

#include "psmove_examples_opengl.h"

#include "psmove.h"
#include "psmove_tracker.h"
#include "psmove_fusion.h"

#define MAX_DRUMS 10

class Vector3D {
    public:
        Vector3D(float x=0, float y=0, float z=0) : x(x), y(y), z(z) {}

        Vector3D &
        operator+=(const Vector3D &other) {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }

        Vector3D &
        operator-=(const Vector3D &other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return *this;
        }

        Vector3D
        operator-(const Vector3D &other) const {
            return Vector3D(x - other.x, y - other.y, z - other.z);
        }

        Vector3D &
        operator*=(float s) { x*=s; y*=s; z*=s; return *this; }

        Vector3D &
        operator/=(float s) { return (*this *= 1./s); }

        float
        length() {
            return sqrtf(x*x+y*y+z*z);
        }

        void
        normalize() {
            *this /= length();
        }

        float x;
        float y;
        float z;
};

class Drum {
    public:
        Drum()
            : pos(),
              radius(2.),
              highlighted(false)
        {}

        void render() {
            if (highlighted) {
                glColor3f(0., .5, 0.);
            } else {
                glColor3f(.5, 0., 0.);
            }
            glTranslatef(pos.x, pos.y, pos.z);
            drawSolidSphere(radius, 10, 10);
        }

        Vector3D pos;
        float radius;
        bool highlighted;
};

class Tracker {
    public:
        Tracker();
        ~Tracker();
        void update();

        void init();
        void render();

        PSMove **m_moves;
        int m_move_count;

        Drum m_drums[MAX_DRUMS];
        int m_drum_count;

        PSMoveTracker *m_tracker;
        PSMoveFusion *m_fusion;
        GLuint m_texture;
};

Tracker::Tracker()
    : m_moves(NULL),
      m_move_count(psmove_count_connected()),
      m_drums(),
      m_drum_count(0),
      m_tracker(psmove_tracker_new()),
      m_fusion(psmove_fusion_new(m_tracker, 1., 1000.))
{
    psmove_tracker_set_mirror(m_tracker, PSMove_True);
    psmove_tracker_set_exposure(m_tracker, Exposure_HIGH);

    m_moves = (PSMove**)calloc(m_move_count, sizeof(PSMove*));
    for (int i=0; i<m_move_count; i++) {
        m_moves[i] = psmove_connect_by_id(i);

        psmove_enable_orientation(m_moves[i], PSMove_True);
        assert(psmove_has_orientation(m_moves[i]));

        while (psmove_tracker_enable(m_tracker, m_moves[i]) != Tracker_CALIBRATED);
    }
}

Tracker::~Tracker()
{
    psmove_fusion_free(m_fusion);
    psmove_tracker_free(m_tracker);
    for (int i=0; i<m_move_count; i++) {
        psmove_disconnect(m_moves[i]);
    }
    free(m_moves);
}

void
Tracker::update()
{
    for (int j=0; j<m_drum_count; j++) {
        m_drums[j].highlighted = false;
    }

    for (int i=0; i<m_move_count; i++) {
        while (psmove_poll(m_moves[i]));

        Vector3D pos;
        psmove_fusion_get_position(m_fusion, m_moves[i],
                &(pos.x), &(pos.y), &(pos.z));

        int buttons = psmove_get_buttons(m_moves[i]);
        if (buttons & Btn_MOVE) {
            psmove_reset_orientation(m_moves[i]);
        } else if (buttons & Btn_PS) {
            exit(0);
        } else if (buttons & Btn_CROSS) {
        } else if (buttons & Btn_CIRCLE) {
        }

        unsigned int pressed, released;
        psmove_get_button_events(m_moves[i], &pressed, &released);
        if (pressed & Btn_CROSS) {
            m_drums[m_drum_count].pos = pos;
            m_drums[m_drum_count].highlighted = true;
            m_drum_count++;
            m_drum_count %= MAX_DRUMS;
        }

        for (int j=0; j<m_drum_count; j++) {
            if (m_drums[j].highlighted) continue;

            if ((m_drums[j].pos - pos).length() < m_drums[j].radius) {
                m_drums[j].highlighted = true;
            }
        }
    }

    psmove_tracker_update_image(m_tracker);
    psmove_tracker_update(m_tracker, NULL);
}

void
Tracker::init()
{
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void
Tracker::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    PSMoveTrackerRGBImage image = psmove_tracker_get_image(m_tracker);

    glEnable(GL_TEXTURE_2D);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width, image.height,
            0, GL_RGB, GL_UNSIGNED_BYTE, image.data);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Draw the camera image, filling the screen */
    glColor3f(1., 1., 1.);
    glBegin(GL_QUADS);
    glTexCoord2f(0., 1.);
    glVertex2f(-1., -1.);
    glTexCoord2f(1., 1.);
    glVertex2f(1., -1.);
    glTexCoord2f(1., 0.);
    glVertex2f(1., 1.);
    glTexCoord2f(0., 0.);
    glVertex2f(-1., 1.);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    /* Clear the depth buffer to allow overdraw */
    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(psmove_fusion_get_projection_matrix(m_fusion));

    for (int i=0; i<m_move_count; i++) {
        glDisable(GL_LIGHTING);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(psmove_fusion_get_modelview_matrix(m_fusion, m_moves[i]));

        glColor3f(1., 0., 0.);
        drawWireCube(1.);
        glColor3f(0., 1., 0.);

        glPushMatrix();
        glScalef(1., 1., 4.5);
        glTranslatef(0., 0., -.5);
        drawWireCube(1.);
        glPopMatrix();

        glColor3f(0., 0., 1.);
        drawWireCube(3.);
    }

    glEnable(GL_LIGHTING);
    glColorMaterial ( GL_FRONT_AND_BACK, GL_EMISSION ) ;
    glEnable ( GL_COLOR_MATERIAL ) ;

    for (int i=0; i<m_drum_count; i++) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        m_drums[i].render();
    }

    glDisable(GL_LIGHTING);
}


class Renderer : public GLFWRenderer {
public:
    Renderer(Tracker &tracker);
    ~Renderer();

    void init();
    void render();

private:
    Tracker &m_tracker;
};

Renderer::Renderer(Tracker &tracker)
    : GLFWRenderer()
    , m_tracker(tracker)
{
}

Renderer::~Renderer()
{
}

void
Renderer::init()
{
    glClearColor(0., 0., 0., 1.);

    glViewport(0, 0, 640, 480);

    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
}

void
Renderer::render()
{
    m_tracker.render();
}

class Main {
    public:
        Main(Tracker &tracker, Renderer &renderer);
        int exec();
    private:
        Tracker &m_tracker;
        Renderer &m_renderer;
};

Main::Main(Tracker &tracker, Renderer &renderer)
    : m_tracker(tracker),
      m_renderer(renderer)
{
}

int
Main::exec()
{
    m_renderer.init();
    m_tracker.init();

    m_renderer.mainloop([&] () {
        m_tracker.update();
        m_renderer.render();
    });

    return 0;
}

int
main(int argc, char *argv[])
{
    Tracker tracker;
    srand(time(NULL));
    Renderer renderer(tracker);
    Main main(tracker, renderer);

    return main.exec();
}

