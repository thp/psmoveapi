#ifndef DEMOVIEW_H
#define DEMOVIEW_H

#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>

typedef struct {
    QGraphicsItem *target;
    int x;
    int y;
} DragOperation;

typedef struct {
    QGraphicsItem *target;
    int x;
    int y;
    float distance;
    float degrees;
} ZoomOperation;

typedef struct {
    QGraphicsItem *target;
    int x;
    int y;
    float q0;
    float q1;
    float q2;
    float q3;
} RotateOperation;

class DemoView : public QGraphicsView
{
    Q_OBJECT

    public:
        DemoView();
        void resizeEvent(QResizeEvent *event);

    public slots:
        void startDrag(int x, int y);
        void updateDrag(int x, int y);
        void stopDrag(int x, int y);

        void startZoom(int x, int y, float distance, float degrees);
        void updateZoom(int x, int y, float distance, float degrees);
        void stopZoom(int x, int y, float distance, float degrees);

        void hover(int idx, int x, int y);

        void startRotation(int id, int x, int y, float q0, float q1, float q2, float q3);
        void updateRotation(int id, float q0, float q1, float q2, float q3);
        void stopRotation(int id);

    private:
        QGraphicsEllipseItem *hovers[2];
        DragOperation drag;
        ZoomOperation zoom;
        RotateOperation rotate[2];

        QGraphicsItem *pinpoint(int x, int y);
        void project(int &x, int &y);
};

#endif
