
/**
 * PS Move API
 * Copyright (c) 2011 Thomas Perl <thp.io/about>
 * All Rights Reserved
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
      _buttons(0)
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
PSMoveQt::pair()
{
    if (psmove_pair(_move)) {
        return true;
    } else {
        return false;
    }
}

int
PSMoveQt::connectionType()
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
PSMoveQt::enabled()
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
PSMoveQt::trigger()
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
PSMoveQt::color()
{
    return _color;
}

void
PSMoveQt::setColor(QColor color)
{
    if (_color != color) {
        _color = color;
        psmove_set_leds(_move, color.red(), color.green(), color.blue());
        psmove_update_leds(_move);

        emit colorChanged();
    }
}

int
PSMoveQt::rumble()
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
    int ax, ay, az, gx, gy, gz, buttons;

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
        buttons = psmove_get_buttons(_move);
        if (buttons != _buttons) {
            int pressed = (buttons & ~_buttons);
            int released = (_buttons & ~buttons);

            for (int i=1; i<=PSMoveQt::T; i <<= 1) {
                if (pressed & i) {
                    emit buttonPressed(i);
                } else if (released & i) {
                    emit buttonReleased(i);
                }
            }

            _buttons = buttons;
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

