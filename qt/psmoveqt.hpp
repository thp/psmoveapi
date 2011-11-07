
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2011 Thomas Perl <m@thp.io>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 **/


#ifndef __PSMOVEQT_HPP
#define __PSMOVEQT_HPP

#include <QtCore>
#include <QtGui>

#include "psmove.h"

class PSMoveQt : public QObject
{
    Q_OBJECT
    Q_ENUMS(ButtonType ConnectionType BatteryChargeNames)

    PSMove *_move;
    QTimer _timer;
    QTimer _colorTimer;

    int _index;
    int _trigger;
    QColor _color;
    int _rumble;
    int _ax;
    int _ay;
    int _az;
    int _gx;
    int _gy;
    int _gz;

    int _mx;
    int _my;
    int _mz;

    int _buttons;
    int _battery;

public:
    PSMoveQt(int index=0);
    ~PSMoveQt();

    static int count();

    bool pair() const;

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

    enum ConnectionType {
        USB = Conn_USB,
        Bluetooth = Conn_Bluetooth,
        Unknown = Conn_Unknown,
    };

    enum BatteryChargeNames {
        BatteryMin = Batt_MIN,
        BatteryMax = Batt_MAX,
        BatteryCharging = Batt_CHARGING,
    };

    int connectionType() const;

#ifdef QT_DECLARATIVE_LIB
    static void registerQML();
#endif

    bool enabled() const;
    void setEnabled(bool enabled);
    int trigger() const;
    void setTrigger(int trigger);
    bool charging() const { return (_battery == BatteryCharging); }
    int battery() const { return (_battery == BatteryCharging)?Batt_MAX:_battery; }
    QColor color() const;
    void setColor(const QColor& color);
    int rumble() const;
    void setRumble(int rumble);

    int ax() const { return _ax; }
    int ay() const { return _ay; }
    int az() const { return _az; }
    int gx() const { return _gx; }
    int gy() const { return _gy; }
    int gz() const { return _gz; }

    int mx() const { return _mx; }
    int my() const { return _my; }
    int mz() const { return _mz; }

    int index() const { return _index; }
    void setIndex(int index);

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
    Q_PROPERTY(int trigger READ trigger WRITE setTrigger NOTIFY triggerChanged)
    Q_PROPERTY(bool charging READ charging NOTIFY chargingChanged)
    Q_PROPERTY(int battery READ battery NOTIFY batteryChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int rumble READ rumble WRITE setRumble NOTIFY rumbleChanged)
    Q_PROPERTY(int ax READ ax NOTIFY accelerometerChanged)
    Q_PROPERTY(int ay READ ay NOTIFY accelerometerChanged)
    Q_PROPERTY(int az READ az NOTIFY accelerometerChanged)
    Q_PROPERTY(int gx READ gx NOTIFY gyroChanged)
    Q_PROPERTY(int gy READ gy NOTIFY gyroChanged)
    Q_PROPERTY(int gz READ gz NOTIFY gyroChanged)

    Q_PROPERTY(int mx READ mx NOTIFY magnetometerChanged)
    Q_PROPERTY(int my READ my NOTIFY magnetometerChanged)
    Q_PROPERTY(int mz READ mz NOTIFY magnetometerChanged)

    Q_PROPERTY(int connectionType READ connectionType NOTIFY indexChanged)

private slots:
    void onTimeout();
    void onColorTimeout();
    void checkColorTimer();

signals:
    void enabledChanged();
    void indexChanged();
    void triggerChanged();
    void chargingChanged();
    void batteryChanged(int battery);
    void colorChanged();
    void rumbleChanged();
    void accelerometerChanged();
    void gyroChanged();

    void magnetometerChanged();

    void buttonPressed(int button);
    void buttonReleased(int button);
};


#endif

