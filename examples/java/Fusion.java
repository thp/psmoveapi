 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2013 Thomas Perl <m@thp.io>
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

import io.thp.psmove.PSMove;
import io.thp.psmove.Frame;
import io.thp.psmove.psmoveapi;

import io.thp.psmove.PSMoveTracker;
import io.thp.psmove.PSMoveFusion;
import io.thp.psmove.PSMoveMatrix4x4;
import io.thp.psmove.Status;
import io.thp.psmove.Button;

import org.lwjgl.LWJGLException;
import org.lwjgl.opengl.Display;
import org.lwjgl.opengl.DisplayMode;
import static org.lwjgl.opengl.GL11.*;
import org.lwjgl.BufferUtils;
import java.nio.FloatBuffer;

public class Fusion {
    private PSMoveTracker tracker;
    private PSMoveFusion fusion;
    private PSMove move;

    Fusion() {
        initPSMove();
        initOpenGL();
    }

    private void initPSMove() {
        if (psmoveapi.count_connected() == 0) {
            System.out.println("Please connect a controller first.");
            System.exit(1);
        }

        tracker = new PSMoveTracker();
        tracker.set_mirror(1);

        fusion = new PSMoveFusion(tracker, 1.f, 1000.f);
        move = new PSMove();
        move.enable_orientation(1);
        move.reset_orientation();

        while (tracker.enable(move) != Status.Tracker_CALIBRATED);
    }

    private void initOpenGL() {
        try {
            Display.setDisplayMode(new DisplayMode(640, 480));
            Display.create();
        } catch (LWJGLException e) {
            e.printStackTrace();
            System.exit(1);
        }

        glEnable(GL_DEPTH_TEST);
    }

    public void run() {
        while (!Display.isCloseRequested()) {
            updatePSMove();
            updateOpenGL();
        }

        Display.destroy();
    }

    private void updatePSMove() {
        while (move.poll() != 0) {}

        tracker.update_image();
        tracker.update();

        if ((move.get_buttons() & Button.Btn_MOVE.swigValue()) != 0) {
            move.reset_orientation();
        }
    }

    private void loadMatrix(int mode, PSMoveMatrix4x4 matrix) {
        FloatBuffer buffer = BufferUtils.createFloatBuffer(16);

        for (int i=0; i<16; i++) {
            buffer.put(matrix.at(i));
        }
        buffer.flip();

        glMatrixMode(mode);
        glLoadMatrix(buffer);
    }

    private void updateOpenGL() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        loadMatrix(GL_PROJECTION, fusion.get_projection_matrix());
        loadMatrix(GL_MODELVIEW, fusion.get_modelview_matrix(move));

        glColor3d(1., 0., 0.);
        glBegin(GL_QUADS);
        glVertex3f(-.5f, -.5f, .5f);
        glVertex3f(-.5f, .5f, .5f);
        glVertex3f(.5f, .5f, .5f);
        glVertex3f(.5f, -.5f, .5f);
        glEnd();

        glColor3d(0., 1., 0.);
        glBegin(GL_QUADS);
        glVertex3f(-.5f, -.5f, -.5f);
        glVertex3f(-.5f, .5f, -.5f);
        glVertex3f(.5f, .5f, -.5f);
        glVertex3f(.5f, -.5f, -.5f);
        glEnd();

        glColor3d(0., 0., 1.);
        glBegin(GL_QUADS);
        glVertex3f(.5f, -.5f, -.5f);
        glVertex3f(.5f, .5f, -.5f);
        glVertex3f(.5f, .5f, .5f);
        glVertex3f(.5f, -.5f, .5f);
        glEnd();

        glColor3d(0., 1., 1.);
        glBegin(GL_QUADS);
        glVertex3f(-.5f, -.5f, -.5f);
        glVertex3f(-.5f, .5f, -.5f);
        glVertex3f(-.5f, .5f, .5f);
        glVertex3f(-.5f, -.5f, .5f);
        glEnd();

        glColor3d(1., 0., 1.);
        glBegin(GL_QUADS);
        glVertex3f(-.5f, .5f, -.5f);
        glVertex3f(.5f, .5f, -.5f);
        glVertex3f(.5f, .5f, .5f);
        glVertex3f(-.5f, .5f, .5f);
        glEnd();

        glColor3d(1., 1., 0.);
        glBegin(GL_QUADS);
        glVertex3f(-.5f, -.5f, -.5f);
        glVertex3f(.5f, -.5f, -.5f);
        glVertex3f(.5f, -.5f, .5f);
        glVertex3f(-.5f, -.5f, .5f);
        glEnd();

        Display.update();
    }

    public static void main(String [] args) {
        Fusion fusion = new Fusion();
        fusion.run();
    }
}

