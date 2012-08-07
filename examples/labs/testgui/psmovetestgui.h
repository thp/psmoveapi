
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
