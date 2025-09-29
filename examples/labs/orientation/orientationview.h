/**
 * PS Move API - An interface for the PS Move Motion Controller
 * (license header unchanged)
 */
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
