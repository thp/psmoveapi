
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


#include "view.h"

#include "psmove.h"

#include <QMatrix4x4>
#include <QDebug>

#include <GL/glu.h>
#include <GL/glut.h>
#include <math.h>

View::View()
    : QGLWidget(),
      m_time(0),
      m_timer(),
      m_quaternion(),
      m_accelerometer(),
      m_buttons(),
      m_trigger(),
      m_position(),
      m_velocity(),
      m_show_controller(),
      m_buttons_old(),
      m_eye()
{
    m_timer.setSingleShot(false);
    m_timer.setInterval(10);
    //m_timer.start();

    connect(&m_timer, SIGNAL(timeout()),
            this, SLOT(updateGL()));
}

View::~View()
{
}

void
View::initializeGL()
{
    glClearColor(0., 0., 0., 1.);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glEnable(GL_CULL_FACE);

    glEnable(GL_LIGHT0);
    GLfloat pos[4] = {-100, 100, 100, 0};
    glLightfv(GL_LIGHT0, GL_POSITION, pos);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glClearDepth(0);
}

void
View::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* http://www.opengl.org/discussion_boards/showthread.php/135646-default-gluPerspective */
    gluPerspective(40, (float)width/(float)height, 500, 10);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(10, 40, 50, 0, 0, -10, 0, 1, 0);
}

void
drawAxis()
{
    int i;

    glBegin(GL_LINES);
    glVertex3f(0, 0, 0);
    glVertex3f(100, 0, 0);

    for (i=-10; i<=10; i++) {
        glVertex3f(i*10, 0, -.5);
        glVertex3f(i*10, 0, .5);
        glVertex3f(i*10, -.5, 0);
        glVertex3f(i*10, .5, 0);
    }

    glVertex3f(0, 0, 0);
    glVertex3f(-100, 0, 0);
    glEnd();
}

void
drawLine(QVector3D from, QVector3D to)
{
    glBegin(GL_LINES);
    glVertex3f(from.x()*10, from.y()*10, from.z()*10);
    glVertex3f(to.x()*10, to.y()*10, to.z()*10);
    glEnd();
}

void
drawVector(QVector3D vector)
{
    drawLine(QVector3D(0, 0, 0), vector);

    glBegin(GL_LINES);
    glVertex3f(vector.x()*10, 0, -1);
    glVertex3f(vector.x()*10, 0, 1);

    glVertex3f(-1, 0, vector.z()*10);
    glVertex3f(1, 0, vector.z()*10);

    glVertex3f(0, vector.y()*10, -1);
    glVertex3f(0, vector.y()*10, 1);
    glEnd();
}

void
renderSphere(QVector3D vector)
{
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);

    glPushMatrix();
    glTranslatef(vector.x()*10, vector.y()*10, vector.z()*10);
    glutSolidSphere(1, 10, 10);
    glPopMatrix();

    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
}

void
renderController(QQuaternion quaternion, int buttons, int trigger)
{
    glPushMatrix();
    QMatrix4x4 mat;
    mat.rotate(quaternion);
    glScalef(4, 4, 4);
    glMultMatrixd(mat.data());



    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);

    /* Bulb */
    if (buttons & Btn_SQUARE) {
        glColor3f(1, 0, 1);
    } else if (buttons & Btn_TRIANGLE) {
        glColor3f(0, 1, 0);
    } else if (buttons & Btn_CROSS) {
        glColor3f(0, 0, 1);
    } else if (buttons & Btn_CIRCLE) {
        glColor3f(1, 0, 0);
    } else {
        glColor3f(1, 1, 1);
    }
    glutSolidSphere(1.5, 20, 20);

    /* Controller body */
    glColor3f(.2, .2, .2);
    glPushMatrix();
    glTranslatef(0, 0, 5);
    glScalef(1, 1, 5);
    glutSolidCube(1.5);
    glPopMatrix();

    /* LED */
    glColor3f(1., 0, 0);
    glPushMatrix();
    glTranslatef(0, .7, 8);
    glutSolidCube(.4);
    glPopMatrix();

    /* Top buttons */
    glPushMatrix();
    if (buttons & Btn_SQUARE) {
        glColor3f(1, .5, .5);
        glTranslatef(0, -.1, 0);
    } else {
        glColor3f(.5, .5, .5);
    }
    glTranslatef(-.6, .7, 3);
    glutSolidCube(.5);
    glPopMatrix();

    glPushMatrix();
    if (buttons & Btn_TRIANGLE) {
        glColor3f(.5, 1, .5);
        glTranslatef(0, -.1, 0);
    } else {
        glColor3f(.5, .5, .5);
    }
    glTranslatef(.6, .7, 3);
    glutSolidCube(.5);
    glPopMatrix();

    glPushMatrix();
    if (buttons & Btn_CROSS) {
        glColor3f(.5, .5, 1);
        glTranslatef(0, -.1, 0);
    } else {
        glColor3f(.5, .5, .5);
    }
    glTranslatef(-.6, .7, 4.1);
    glutSolidCube(.5);
    glPopMatrix();

    glPushMatrix();
    if (buttons & Btn_CIRCLE) {
        glColor3f(1, .3, .3);
        glTranslatef(0, -.1, 0);
    } else {
        glColor3f(.5, .5, .5);
    }
    glTranslatef(.6, .7, 4.1);
    glutSolidCube(.5);
    glPopMatrix();


    /* Trigger */
    float trig = (float)trigger / 255.;
    glPushMatrix();
    if (buttons & Btn_T) {
        glColor3f(.8, .8, .8);
    } else {
        glColor3f(.5, .5, .5);
    }
    glTranslatef(0, -.5, 2.5 + .5 * trig);
    glRotatef(-45 * trig, 1, 0, 0);
    glScalef(1.5, 1, 1);
    glutSolidCube(.8);
    glPopMatrix();


    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);


    glPopMatrix();
}

void
View::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 mat;
    mat.rotate(m_quaternion);

    if (m_buttons & (Btn_START | Btn_SELECT)) {
        //qDebug() << m_time;
        m_time += 0.01;
    }

    if (m_buttons & Btn_START) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(30*sinf(m_time), 40, 90*cosf(m_time), 0, 0, -10*cosf(m_time), 0, 1, 0);
    } else if (m_buttons & Btn_SELECT) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        m_time -= .005;
        QVector3D eye(30*sinf(m_time), 40+((float)m_trigger/255.)*40, 90*cosf(m_time));
        //QVector3D eye(m_eye);
        QVector3D direction_diff(mat.map(QVector3D(0, 0, -1)));
        QVector3D direction(eye + direction_diff);
        m_eye += direction_diff / 5;
        QVector3D up(mat.map(QVector3D(0, 1, 0)));
        gluLookAt(eye.x(), eye.y(), eye.z(),
                direction.x(), direction.y(), direction.z(),
                up.x(), up.y(), up.z());
    }

    glColor3f(1, 0, 0);
    drawAxis();

    glColor3f(0, 1, 0);
    glPushMatrix();
    glRotatef(90, 0, 0, 1);
    drawAxis();
    glPopMatrix();

    glColor3f(0, 0, 1);
    glPushMatrix();
    glRotatef(90, 0, -1, 0);
    drawAxis();
    glPopMatrix();

    if (fabsf(1.-m_accelerometer.length()) < .05) {
        glColor3f(0, 1, 1);
    } else {
        glColor3f(1, 0, 1);
    }
    drawVector(m_accelerometer);

    glColor3f(1, 1, 1);

    QVector3D ax_negative(mat.map(QVector3D(1, 0, 0)));
    ax_negative = QVector3D(ax_negative.y(), 0, 0);
    drawVector(ax_negative);

    QVector3D ay_negative(mat.map(QVector3D(0, 0, 1)));
    ay_negative = QVector3D(0, ay_negative.y(), 0);
    drawVector(ay_negative);

    QVector3D az_negative(mat.map(QVector3D(0, 1, 0)));
    az_negative = QVector3D(0, 0, az_negative.y());
    drawVector(az_negative);

    QVector3D accelerometer_negative = QVector3D(
            ax_negative.x(),
            ay_negative.y(),
            az_negative.z());

    QVector3D accelerometer_movement(m_accelerometer +
            accelerometer_negative);

    accelerometer_movement = QVector3D(
            powf(-accelerometer_movement.x(), 3),
            powf(-accelerometer_movement.z(), 3),
            powf(-accelerometer_movement.y(), 3));

    /* Resulting accelerometer vector */
    glColor3f(1, 0, 1);
    drawVector(mat.map(accelerometer_movement));

    m_velocity += accelerometer_movement / 1000;
    m_velocity *= .9;
    //m_velocity += QVector3D(m_accelerometer.x(), 0, m_accelerometer.y()) / 1000;
    m_position += m_velocity;
    if (m_buttons & Btn_MOVE) {
        m_velocity = QVector3D(0, 0, 0);
        m_position *= .5;
    }
    //qDebug() << m_position;
    renderSphere(m_position);

    QList< QPair<QVector3D,QVector3D> > lines;

#define ADD_LINE(a,b,c,x,y,z) \
    lines << QPair<QVector3D,QVector3D>(QVector3D(a,b,c),QVector3D(x,y,z))

    ADD_LINE(1, 0, 0, 0, 0, 0);
    ADD_LINE(0, 1, 0, 0, 0, 0);
    ADD_LINE(0, 0, 1, 0, 0, 0);

    ADD_LINE(0, 1, 1, 1, 1, 1);
    ADD_LINE(1, 0, 1, 1, 1, 1);
    ADD_LINE(1, 1, 0, 1, 1, 1);

    ADD_LINE(0, 0, 1, 1, 0, 1);
    ADD_LINE(1, 0, 1, 1, 0, 0);
    ADD_LINE(1, 0, 0, 1, 1, 0);
    ADD_LINE(1, 1, 0, 0, 1, 0);
    ADD_LINE(0, 1, 0, 0, 1, 1);
    ADD_LINE(0, 1, 1, 0, 0, 1);

    if (m_show_controller) {
        renderController(m_quaternion, m_buttons, m_trigger);
    }

    if ((m_buttons ^ m_buttons_old) & Btn_PS) {
        if (m_buttons & Btn_PS) {
            m_show_controller = !m_show_controller;
        }
    }

    glColor3f(1, 1, 0);

    QPair<QVector3D,QVector3D> line;
    foreach (line, lines) {
        drawLine(mat.map(line.first), mat.map(line.second));
    }

    glColor3f(1, 1, 1);
    QString statusLine;
    statusLine = QString("acc: %1/%2/%3")
        .arg(m_accelerometer.x(), 6, 'f', 3)
        .arg(m_accelerometer.y(), 6, 'f', 3)
        .arg(m_accelerometer.z(), 6, 'f', 3);
    renderText(10, 20, statusLine);

    QList<QVector3D> spheres;

    spheres << QVector3D(0, 0, -1);
    spheres << QVector3D(0, 0, 1);
    spheres << QVector3D(0, -1, 0);
    spheres << QVector3D(0, 1, 0);
    spheres << QVector3D(-1, 0, 0);
    spheres << QVector3D(1, 0, 0);

    QVector3D sphere;
    foreach (sphere, spheres) {
        if ((sphere - m_accelerometer).length() < .1) {
            glColor3f(0, 1, 0);
        } else {
            glColor3f(.5, .5, .5);
        }

        renderSphere(sphere);
    }

    glColor3f(1, 1, 0);
    int a, b, c;
    for (a=-5; a<=5; a+=4) {
        for (b=-5; b<=5; b+=4) {
            for (c=-5; c<=5; c+=4) {
                renderSphere(QVector3D(a, b, c));
            }
        }
    }

    glColor3f(0, 1, .5);
    renderSphere(m_accelerometer);
}

void
View::updateQuaternion(QQuaternion quaternion)
{
    m_quaternion = quaternion;
}

void
View::updateAccelerometer(QVector3D accelerometer)
{
    m_accelerometer = accelerometer;

    m_accelerometer.setX(-m_accelerometer.x());
    m_accelerometer.setZ(-m_accelerometer.z());

    updateGL();
}

void
View::updateButtons(int buttons, int trigger)
{
    m_buttons_old = m_buttons;
    m_buttons = buttons;
    m_trigger = trigger;
}

