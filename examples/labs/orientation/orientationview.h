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

#ifndef ORIENTATIONVIEW_H
#define ORIENTATIONVIEW_H

#include <QWidget>
#include <QMatrix4x4>
#include <QHBoxLayout>

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>

#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QTorusMesh>     // use a torus for a nice visual
#include <Qt3DExtras/QSphereMesh>    // alt: sphere
#include <Qt3DRender/QCamera>

class OrientationView : public QWidget
{
    Q_OBJECT
public:
    explicit OrientationView(QWidget *parent = nullptr);
    ~OrientationView();

public slots:
    void orientation(qreal a, qreal b, qreal c, qreal d,
                     qreal scale, qreal x, qreal y);

private:
    Qt3DExtras::Qt3DWindow      *m_view = nullptr;
    QWidget                     *m_container = nullptr;
    Qt3DCore::QEntity           *m_root = nullptr;
    Qt3DCore::QEntity           *m_objectEntity = nullptr;
    Qt3DCore::QTransform        *m_transform = nullptr;
};

#endif // ORIENTATIONVIEW_H
