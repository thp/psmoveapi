#ifndef PSMOVETESTGUI_H
#define PSMOVETESTGUI_H

#include <QMainWindow>

namespace Ui {
    class PSMoveTestGUI;
}

class PSMoveTestGUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit PSMoveTestGUI(QWidget *parent = 0);
    ~PSMoveTestGUI();

private slots:
    void on_actionQuit_activated();

private:
    Ui::PSMoveTestGUI *ui;
};

#endif // PSMOVETESTGUI_H
