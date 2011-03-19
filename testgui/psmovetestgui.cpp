#include "psmovetestgui.h"
#include "ui_psmovetestgui.h"

PSMoveTestGUI::PSMoveTestGUI(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PSMoveTestGUI)
{
    ui->setupUi(this);
}

PSMoveTestGUI::~PSMoveTestGUI()
{
    delete ui;
}

void PSMoveTestGUI::on_actionQuit_activated()
{
    QApplication::quit();
}
