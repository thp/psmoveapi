#ifndef ORIENTATIONVIEW_H
#define ORIENTATIONVIEW_H

#include <QWidget>

class PaintView : public QWidget
{
    Q_OBJECT
public:
    PaintView(QWidget *parent=0);
    ~PaintView();

protected:
    void paintEvent(QPaintEvent *event);

private:
    QPixmap m_painting;
    QPoint m_cursor;
    QColor m_color;

    bool m_has_old;
    QPoint m_old;

public slots:
    void orientation(qreal a, qreal b, qreal c, qreal d,
            qreal scale, qreal x, qreal y, qreal trigger);
    void newcolor(int r, int g, int b);
};

#endif
