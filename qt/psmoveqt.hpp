
/**
 * PS Move API
 * Copyright (c) 2011 Thomas Perl <thp.io/about>
 * All Rights Reserved
 **/


#ifndef __PSMOVEQT_HPP
#define __PSMOVEQT_HPP

#include <QtCore>
#include <QtGui>

#include "psmove.h"

class PSMoveQt : public QObject
{
    Q_OBJECT
    Q_ENUMS(ButtonType)

    PSMove *_move;
    QTimer _timer;
    QTimer _colorTimer;

    int _trigger;
    QColor _color;
    int _rumble;
    int _ax;
    int _ay;
    int _az;
    int _gx;
    int _gy;
    int _gz;
    int _buttons;

public:
    PSMoveQt();
    ~PSMoveQt();

    enum ButtonType {
        Triangle = Btn_TRIANGLE,
        Circle = Btn_CIRCLE,
        Cross = Btn_CROSS,
        Square = Btn_SQUARE,
        Select = Btn_SELECT,
        Start = Btn_START,
        PS = Btn_PS,
        Move = Btn_MOVE,
        T = Btn_T,
    };

#ifdef QT_DECLARATIVE_LIB
    static void registerQML();
#endif

    bool enabled();
    void setEnabled(bool enabled);
    int trigger();
    void setTrigger(int trigger);
    QColor color();
    void setColor(QColor color);
    int rumble();
    void setRumble(int rumble);

    int ax() { return _ax; }
    int ay() { return _ay; }
    int az() { return _az; }
    int gx() { return _gx; }
    int gy() { return _gy; }
    int gz() { return _gz; }

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int trigger READ trigger WRITE setTrigger NOTIFY triggerChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int rumble READ rumble WRITE setRumble NOTIFY rumbleChanged)
    Q_PROPERTY(int ax READ ax NOTIFY accelerometerChanged)
    Q_PROPERTY(int ay READ ay NOTIFY accelerometerChanged)
    Q_PROPERTY(int az READ az NOTIFY accelerometerChanged)
    Q_PROPERTY(int gx READ gx NOTIFY gyroChanged)
    Q_PROPERTY(int gy READ gy NOTIFY gyroChanged)
    Q_PROPERTY(int gz READ gz NOTIFY gyroChanged)

private slots:
    void onTimeout();
    void onColorTimeout();
    void checkColorTimer();

signals:
    void enabledChanged();
    void triggerChanged();
    void colorChanged();
    void rumbleChanged();
    void accelerometerChanged();
    void gyroChanged();
    void buttonPressed(int button);
    void buttonReleased(int button);
};


#endif

