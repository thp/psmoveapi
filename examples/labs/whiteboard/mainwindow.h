#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QTimer>
#include <QPainterPath>

#include "mapping.h"

#include "psmove.h"
#include "psmove_tracker.h"

class MainWindow : public QWidget
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);
    ~MainWindow();

private slots:
    void timeout();

private:
    void syncRect();

private:
    QPointF m_mousePos;
    Mapping m_mapping;

    QPointF m_points[4];
    int m_pointsOffset;

    PSMove *m_move;
    PSMoveTracker *m_tracker;

    QTimer m_timer;

    QPainterPath m_path;

    QRect m_rect;
    int m_rectOffset;
};

#endif // MAINWINDOW_H
