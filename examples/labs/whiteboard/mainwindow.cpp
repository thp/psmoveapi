#include "mainwindow.h"

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>

#define DEFAULT_BORDER 20

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    m_mousePos(0, 0),
    m_mapping(),
    m_points(),
    m_pointsOffset(0),
    m_move(psmove_connect()),
    m_tracker(psmove_tracker_new()),
    m_timer(),
    m_path(),
    m_rect(rect()),
    m_rectOffset(0)
{
    m_points[0] = QPointF(1, 1);
    m_points[1] = QPointF(640, 0);
    m_points[2] = QPointF(640, 480);
    m_points[3] = QPointF(0, 480);
    m_mapping.set(m_points);

    setMouseTracking(true);

    if (m_move) {
        while (psmove_tracker_enable(m_tracker, m_move) != Tracker_CALIBRATED);
    }

    QObject::connect(&m_timer, SIGNAL(timeout()),
                     this, SLOT(timeout()));
    m_timer.start(1);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    painter.setPen(Qt::blue);
    painter.drawEllipse(m_mapping.map(m_mousePos), 5, 5);
    painter.drawRect(m_mapping.rect());

    painter.setPen(Qt::red);
    painter.drawEllipse(m_mousePos.toPoint(), 5, 5);
    painter.drawPolygon(m_points, 4);

    QPen drawPen(Qt::white, 20, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(drawPen);
    painter.drawPath(m_path);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    m_mousePos = event->posF();
    update();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    m_rect = rect().adjusted(DEFAULT_BORDER, DEFAULT_BORDER,
                             -DEFAULT_BORDER, -DEFAULT_BORDER);
    syncRect();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
        case Qt::LeftButton:
            m_points[m_pointsOffset] = event->posF();
            m_pointsOffset = (m_pointsOffset + 1) % 4;
            m_mapping.set(m_points);
            break;
        case Qt::RightButton:
            if (m_rectOffset == 0) {
                m_rect.setTopLeft(event->pos());
            } else {
                m_rect.setBottomRight(event->pos());
            }
            m_rectOffset = (m_rectOffset + 1) % 2;
            syncRect();
            break;
        default:
            break;
    }

    update();
}

MainWindow::~MainWindow()
{
    psmove_tracker_free(m_tracker);
    psmove_disconnect(m_move);
}

void MainWindow::timeout()
{
    psmove_tracker_update_image(m_tracker);
    psmove_tracker_update(m_tracker, m_move);

    float x, y;
    if (psmove_tracker_get_position(m_tracker, m_move, &x, &y, NULL)) {
        QPointF currentPos(x, y);
        unsigned int buttons = 0;
        while (psmove_poll(m_move)) {
            unsigned int pressed;
            psmove_get_button_events(m_move, &pressed, NULL);
            if (pressed & Btn_MOVE) {
                m_points[m_pointsOffset] = currentPos;
                m_pointsOffset = (m_pointsOffset + 1) % 4;
                m_mapping.set(m_points);
            }
            if (pressed & Btn_T) {
                m_path.moveTo(m_mapping.map(currentPos));
            }
            if (pressed & Btn_CIRCLE) {
                m_path = QPainterPath();
            }
            if (pressed & Btn_PS) {
                QApplication::quit();
            }
            buttons = psmove_get_buttons(m_move);
        }
        if (buttons & Btn_T) {
            m_path.lineTo(m_mapping.map(currentPos));
        }

        m_mousePos = QPointF(x, y);

        update();
    }

}

void MainWindow::syncRect()
{
    m_mapping.setRect(m_rect);
}
