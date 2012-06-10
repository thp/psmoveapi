#ifndef MOVEGRAPH_H
#define MOVEGRAPH_H

#include <QtGui>

#include "psmove.h"

class Reading {
    public:
        Reading() : x(0), y(0), z(0) {}
        int x, y, z;
};

class MoveGraph : public QWidget {
    Q_OBJECT

    public:
        MoveGraph();

    private:
        void insertReading(int x, int y, int z);

    protected:
        virtual void paintEvent(QPaintEvent *event);

    public slots:
        void readSensors();

    private:
        PSMove *move;
        Reading readings[1024];
        int offset;
};

#endif
