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

#include "orientationview.h"

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QForwardRenderer>


#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QTorusMesh>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DRender/QCamera>

#include <QQuaternion>
#include <QVector3D>
#include <QSurfaceFormat>
#include <QSizePolicy>

OrientationView::OrientationView(QWidget *parent)
    : QWidget(parent)
{
    // 3D window
    m_view = new Qt3DExtras::Qt3DWindow();
    auto *fg = qobject_cast<Qt3DExtras::QForwardRenderer*>(m_view->defaultFrameGraph());
    if (fg) fg->setClearColor(QColor(30, 30, 35));
    m_container = QWidget::createWindowContainer(m_view, this);
    m_container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_container);
    setLayout(layout);

    // Root entity
    m_root = new Qt3DCore::QEntity();

    // Camera
    Qt3DRender::QCamera *camera = m_view->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(0.0f, 0.0f, 6.0f));
    camera->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));

    // Camera controls
    auto *camController = new Qt3DExtras::QOrbitCameraController(m_root);
    camController->setLinearSpeed( 50.0f );
    camController->setLookSpeed(  180.0f );
    camController->setCamera(camera);

    // Visual geometry (torus looks nicer than a cube)
    auto *mesh = new Qt3DExtras::QTorusMesh();
    mesh->setRadius(1.0f);
    mesh->setMinorRadius(0.3f);
    mesh->setRings(64);
    mesh->setSlices(32);

    // Transform we’ll update from PS Move orientation
    m_transform = new Qt3DCore::QTransform();
    m_transform->setScale(1.0f);

    // Material
    auto *material = new Qt3DExtras::QPhongMaterial();
    material->setShininess(80.0f);

    // Entity
    m_objectEntity = new Qt3DCore::QEntity(m_root);
    m_objectEntity->addComponent(mesh);
    m_objectEntity->addComponent(m_transform);
    m_objectEntity->addComponent(material);

    // Set the scene
    m_view->setRootEntity(m_root);
}

OrientationView::~OrientationView()
{
    // Qt3D objects parented under m_root will be freed automatically
}

void OrientationView::orientation(qreal a, qreal b, qreal c, qreal d,
                                  qreal scale, qreal x, qreal y)
{
    if (!m_transform) return;

    // Map original example’s 2D offsets to world translation
    // Original used translate(QVector3D(x*2, y*1.5, 0))
    const QVector3D pos(x * 2.0f, y * 1.5f, 0.0f);
    m_transform->setTranslation(pos);

    // Scale
    m_transform->setScale(std::max<qreal>(0.05, scale)); // clamp to avoid disappearing

    // Orientation: PS Move provides quaternion (w, x, y, z) in example order (a,b,c,d)
    // Qt expects (scalar, x, y, z) too, matching QQuaternion(a,b,c,d).
    const QQuaternion q(a, b, c, d);
    m_transform->setRotation(q);
}
