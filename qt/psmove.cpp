
/**
 * PS Move API
 * Copyright (c) 2011 Thomas Perl <thp.io/about>
 * All Rights Reserved
 **/

#include "psmove.hpp"

#include <QtDeclarative>
#include <QtDebug>

PSMoveQt::PSMoveQt()
{
    _move = psmove_connect();
    _timer.setInterval(10);
    /* FIXME: initialize fields */
    _buttons = 0;

    /**
     * Re-sending the LED color value every 4 secs should be enough.
     * For this we need a timer, and when color and/or rumble are
     * set to a non-zero value, we need to activate it.
     **/
    _colorTimer.setInterval(4000);
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
    /* Switch off LEDs + rumble on exit */
    setColor(Qt::black);
    setRumble(0);

    psmove_disconnect(_move);
}

void
PSMoveQt::registerQML()
{
    qmlRegisterType<PSMoveQt>("com.thpinfo.psmove", 1, 0, "PSMove");
}

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
    int gx, gy, gz, buttons;

    while (psmove_poll(_move)) {
        setTrigger(psmove_get_trigger(_move));
        psmove_get_gyroscope(_move, &gx, &gy, &gz);
        if (gx != _gx || gy != _gy || gz != _gz) {
            _gx = gx;
            _gy = gy;
            _gz = gz;
            emit gyroChanged();
        }
        buttons = psmove_get_buttons(_move);
        if (buttons != _buttons) {
            /* FIXME: Make more generic + for all buttons */
            if (buttons & Btn_MOVE && !(_buttons & Btn_MOVE)) {
                emit buttonPressed(PSMoveQt::Move);
            } else if (!(buttons & Btn_MOVE) && _buttons & Btn_MOVE) {
                emit buttonReleased(PSMoveQt::Move);
            }
            if (buttons & Btn_PS && !(_buttons & Btn_PS)) {
                emit buttonPressed(PSMoveQt::PS);
            } else if (!(buttons & Btn_PS) && _buttons & Btn_PS) {
                emit buttonReleased(PSMoveQt::PS);
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

