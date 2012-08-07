
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

void OrientationView::orientation(qreal a, qreal b, qreal c, qreal d,
        qreal scale, qreal x, qreal y)
{
    QMatrix4x4 mat;
    mat.translate(QVector3D(x*2, y*1.5, 0));
    mat.scale(scale);
    mat.rotate(QQuaternion(a, b, c, d));
    teapot->setLocalTransform(mat);
    update();
}

