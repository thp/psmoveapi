
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


#include "psmoveqt.hpp"

#ifdef QT_DECLARATIVE_LIB
#  include <QtDeclarative>
#endif

/* Update interval for sensor readings and LED setting */
#define INTERVAL_SENSORS 10
#define INTERVAL_RGBLEDS 4000

PSMoveQt::PSMoveQt(int index)
    : _move(psmove_connect_by_id(index)),
      _timer(),
      _colorTimer(),
      _index(index),
      _trigger(0),
      _color(Qt::black),
      _rumble(0),
      _ax(0),
      _ay(0),
      _az(0),
      _gx(0),
      _gy(0),
      _gz(0),

      _mx(0),
      _my(0),
      _mz(0),

      _buttons(0),
      _battery(0)
{
    _timer.setInterval(INTERVAL_SENSORS);

    /**
     * Re-sending the LED color value every 4 secs should be enough.
     * For this we need a timer, and when color and/or rumble are
     * set to a non-zero value, we need to activate it.
     **/
    _colorTimer.setInterval(INTERVAL_RGBLEDS);
    connect(this, SIGNAL(enabledChanged()),
            this, SLOT(checkColorTimer()));
    connect(this, SIGNAL(colorChanged()),
            this, SLOT(checkColorTimer()));
    connect(this, SIGNAL(rumbleChanged()),
            this, SLOT(checkColorTimer()));

    connect(&_timer, SIGNAL(timeout()),
            this, SLOT(onTimeout()));
    connect(&_colorTimer, SIGNAL(timeout()),
            this, SLOT(onColorTimeout()));
}

PSMoveQt::~PSMoveQt()
{
    if (_move != NULL) {
        /* Switch off LEDs + rumble on exit */
        setColor(Qt::black);
        setRumble(0);

        psmove_disconnect(_move);
    }
}

int
PSMoveQt::count()
{
    return psmove_count_connected();
}

bool
PSMoveQt::pair() const
{
    if (psmove_pair(_move)) {
        return true;
    } else {
        return false;
    }
}

int
PSMoveQt::connectionType() const
{
    switch (psmove_connection_type(_move)) {
        case Conn_USB:
            return PSMoveQt::USB;
        case Conn_Bluetooth:
            return PSMoveQt::Bluetooth;
        default:
            return PSMoveQt::Unknown;
    }
}

#ifdef QT_DECLARATIVE_LIB
void
PSMoveQt::registerQML()
{
    qmlRegisterType<PSMoveQt>("com.thpinfo.psmove", 1, 0, "PSMove");
}
#endif

bool
PSMoveQt::enabled() const
{
    return _timer.isActive();
}

void
PSMoveQt::setEnabled(bool enabled)
{
    if (enabled && !_timer.isActive()) {
        /* Activate */
        _timer.start();
        emit enabledChanged();
    } else if (!enabled && _timer.isActive()) {
        /* Deactivate */
        _timer.stop();
        emit enabledChanged();
    }
}

int
PSMoveQt::trigger() const
{
    return _trigger;
}

void
PSMoveQt::setTrigger(int trigger)
{
    if (_trigger != trigger) {
        _trigger = trigger;
        emit triggerChanged();
    }
}

void
PSMoveQt::setIndex(int index)
{
    if (_index != index) {
        _index = index;
        PSMove *old = _move;
        _move = psmove_connect_by_id(_index);
        psmove_disconnect(old);
        emit indexChanged();
    }
}

QColor
PSMoveQt::color() const
{
    return _color;
}

void
PSMoveQt::setColor(const QColor& color)
{
    if (_color != color) {
        _color = color;
        psmove_set_leds(_move, color.red(), color.green(), color.blue());
        psmove_update_leds(_move);

        emit colorChanged();
    }
}

int
PSMoveQt::rumble() const
{
    return _rumble;
}

void
PSMoveQt::setRumble(int rumble)
{
    if (_rumble != rumble) {
        _rumble = rumble;
        psmove_set_rumble(_move, _rumble);
        psmove_update_leds(_move);

        emit rumbleChanged();
    }
}

void
PSMoveQt::onTimeout()
{
    int ax, ay, az, gx, gy, gz, mx, my, mz, buttons, battery;

    while (psmove_poll(_move)) {
        setTrigger(psmove_get_trigger(_move));
        psmove_get_gyroscope(_move, &gx, &gy, &gz);
        if (gx != _gx || gy != _gy || gz != _gz) {
            _gx = gx;
            _gy = gy;
            _gz = gz;
            emit gyroChanged();
        }
        psmove_get_accelerometer(_move, &ax, &ay, &az);
        if (ax != _ax || ay != _ay || az != _az) {
            _ax = ax;
            _ay = ay;
            _az = az;
            emit accelerometerChanged();
        }

        psmove_get_magnetometer(_move, &mx, &my, &mz);
        if (mx != _mx || my != _my || mz != _mz) {
            _mx = mx;
            _my = my;
            _mz = mz;
            emit magnetometerChanged();
        }

        buttons = psmove_get_buttons(_move);
        if (buttons != _buttons) {
            unsigned int pressed, released;
            psmove_get_button_events(_move, &pressed, &released);

            for (int i=1; i<=PSMoveQt::T; i <<= 1) {
                if (pressed & i) {
                    emit buttonPressed(i);
                } else if (released & i) {
                    emit buttonReleased(i);
                }
            }

            _buttons = buttons;
        }

        battery = psmove_get_battery(_move);
        if (battery != _battery) {
            bool charging = (battery == Batt_CHARGING ||
                    _battery == Batt_CHARGING);

            _battery = battery;

            if (charging) {
                emit chargingChanged();
            } else if (_battery != Batt_CHARGING) {
                emit batteryChanged(_battery);
            }
        }
    }
}

void
PSMoveQt::onColorTimeout()
{
    /* Re-send the LED color data so it doesn't go off */
    psmove_update_leds(_move);
}

void
PSMoveQt::checkColorTimer()
{
    bool shouldRun = _timer.isActive() &&
        ((_color.red() + _color.green() + _color.blue() + _rumble) > 0);

    if (shouldRun && !_colorTimer.isActive()) {
        /* Enable color timer */
        _colorTimer.start();
    } else if (!shouldRun && _colorTimer.isActive()) {
        /* Disable color timer */
        _colorTimer.stop();
    }
}

