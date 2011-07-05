
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (C) 2011 Thomas Perl <m@thp.io>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
    void buttonPressed(int button);
    void buttonReleased(int button);
};


#endif

