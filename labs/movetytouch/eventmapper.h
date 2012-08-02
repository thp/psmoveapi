#ifndef EVENTMAPPER_H
#define EVENTMAPPER_H

#include <QObject>
#include <QPoint>

typedef struct {
    int x;
    int y;
} Point;

class EventMapper : public QObject
{
    Q_OBJECT

    public:
        EventMapper();

    public slots:
        void pressed(int id, int x, int y);
        void motion(int id, int x, int y);
        void released(int id, int x, int y);

    signals:
        /* Drag'n'drop */
        void startDrag(int x, int y);
        void updateDrag(int x, int y);
        void stopDrag(int x, int y);

        /* Rotation and zoom */
        void startZoom(int x, int y, float distance, float degrees);
        void updateZoom(int x, int y, float distance, float degrees);
        void stopZoom(int x, int y, float distance, float degrees);

    private:
        Point center();
        float distance();
        float degrees();

        Point points[2];
        int activePoints;
};

#endif
