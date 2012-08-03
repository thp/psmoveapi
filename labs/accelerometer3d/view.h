#ifndef VIEW_H
#define VIEW_H

#include <QGLWidget>
#include <QTimer>

#include <QQuaternion>
#include <QVector3D>

class View : public QGLWidget
{
    Q_OBJECT

public:
    View();
    ~View();

protected:
    void initializeGL();
    void resizeGL(int width, int height);
    void paintGL();

public slots:
    void updateQuaternion(QQuaternion quaternion);
    void updateAccelerometer(QVector3D accelerometer);
    void updateButtons(int buttons, int trigger);

private:
    float m_time;
    QTimer m_timer;
    QQuaternion m_quaternion;
    QVector3D m_accelerometer;
    int m_buttons;
    int m_trigger;
};

#endif
