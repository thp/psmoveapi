#ifndef ORIENTATIONVIEW_H
#define ORIENTATIONVIEW_H

#include "Qt3D/qglview.h"

class QGLSceneNode;

class OrientationView : public QGLView
{
    Q_OBJECT
public:
    OrientationView(QWidget *parent = 0) : QGLView(parent), teapot(0) {}
    ~OrientationView();

protected:
    void initializeGL(QGLPainter *painter);
    void paintGL(QGLPainter *painter);

private:
    QGLSceneNode *teapot;

public slots:
    void orientation(qreal a, qreal b, qreal c, qreal d);
};

#endif
