
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

#ifndef PSMOVETESTGUI_H
#define PSMOVETESTGUI_H

#include <QMainWindow>

#include "psmoveqt.hpp"

#include "psmovesensorscene.h"

namespace Ui {
    class PSMoveTestGUI;
}

class PSMoveTestGUI : public QMainWindow
{
    Q_OBJECT

    PSMoveQt *psm;
    PSMoveSensorScene *scene;
    QColor colorLEDs;

    int _chosenIndex;
    PSMoveQt *_red;
    PSMoveQt *_green;

public:
    explicit PSMoveTestGUI(QWidget *parent = 0);
    ~PSMoveTestGUI();

    void reconnectByIndex(int index);

public slots:
    void setTrigger();
    void onButtonPressed(int button);
    void onButtonReleased(int button);

private slots:
    void setColor(QColor color);
    void setButton(int button, bool pressed);
    void readAccelerometer();
    void readGyro();

    void on_checkboxEnable_toggled(bool checked);
    void on_buttonLEDs_clicked();
    void on_sliderRumble_sliderMoved(int position);
    void on_buttonBluetoothSet_clicked();

private:
    Ui::PSMoveTestGUI *ui;
};

#endif // PSMOVETESTGUI_H
