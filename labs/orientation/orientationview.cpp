
#include "orientationview.h"
#include "qglbuilder.h"
#include "qglscenenode.h"
#include "qglteapot.h"
#include "qquaternion.h"
#include "math.h"

#include <QDebug>

void OrientationView::initializeGL(QGLPainter *painter)
{
    painter->setStandardEffect(QGL::LitMaterial);
    QGLBuilder builder;
    builder << QGLTeapot();
    teapot = builder.finalizedSceneNode();
}

OrientationView::~OrientationView()
{
    delete teapot;
}

void OrientationView::paintGL(QGLPainter *painter)
{
    teapot->draw(painter);
}

void OrientationView::orientation(qreal a, qreal b, qreal c, qreal d)
{
    QMatrix4x4 mat;
    mat.rotate(-90., 1., 0., 0.);
    mat.rotate(QQuaternion(a, b, c, d));
    teapot->setLocalTransform(mat);
    update();
}

