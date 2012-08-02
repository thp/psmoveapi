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

class DemoView : public QGraphicsView
{
    Q_OBJECT

    public:
        DemoView();

    public slots:
        void startDrag(int x, int y);
        void updateDrag(int x, int y);
        void stopDrag(int x, int y);

        void startZoom(int x, int y, float distance, float degrees);
        void updateZoom(int x, int y, float distance, float degrees);
        void stopZoom(int x, int y, float distance, float degrees);

        void hover(int idx, int x, int y);

    private:
        QGraphicsEllipseItem *hovers[2];
        DragOperation drag;
        ZoomOperation zoom;

        QGraphicsItem *pinpoint(int x, int y);

};

#endif
