#ifndef PSMOVETESTGUI_H
#define PSMOVETESTGUI_H

#include <QMainWindow>

#include "psmoveqt.hpp"

namespace Ui {
    class PSMoveTestGUI;
}

class PSMoveTestGUI : public QMainWindow
{
    Q_OBJECT

    PSMoveQt *psm;
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

    void on_checkboxEnable_toggled(bool checked);
    void on_buttonLEDs_clicked();
    void on_buttonQuit_clicked();

    void on_sliderRumble_sliderMoved(int position);

    void on_buttonBluetoothSet_clicked();

private:
    Ui::PSMoveTestGUI *ui;
};

#endif // PSMOVETESTGUI_H
