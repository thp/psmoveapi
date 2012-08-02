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
    QPixmap m_painting_backup;
    QPoint m_cursor;
    QColor m_color;

    bool m_has_old;
    QPoint m_old;

public slots:
    void newposition(qreal scale, qreal x, qreal y, qreal trigger);
    void backup_frame();
    void restore_frame();
    void newcolor(int r, int g, int b);
};

#endif
