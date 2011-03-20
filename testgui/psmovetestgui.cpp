#include "psmovetestgui.h"
#include "ui_psmovetestgui.h"

#include "psmovesensorscene.h"

PSMoveTestGUI::PSMoveTestGUI(QWidget *parent) :
    QMainWindow(parent),
    psm(NULL),
    colorLEDs(Qt::black),
    ui(new Ui::PSMoveTestGUI)
{
    ui->setupUi(this);
    psm = new PSMoveQt();

    ui->viewSensors->setScene(new PSMoveSensorScene(this, psm));
    on_checkboxEnable_toggled(false);
    setColor(Qt::red);

    connect(psm, SIGNAL(triggerChanged()),
            this, SLOT(setTrigger()));

    connect(psm, SIGNAL(buttonPressed(int)),
            this, SLOT(onButtonPressed(int)));

    connect(psm, SIGNAL(buttonReleased(int)),
            this, SLOT(onButtonReleased(int)));
}

PSMoveTestGUI::~PSMoveTestGUI()
{
    delete ui;
    delete psm;
}

void PSMoveTestGUI::on_checkboxEnable_toggled(bool checked)
{
    ui->groupLEDs->setEnabled(checked);
    ui->groupRumble->setEnabled(checked);
    ui->groupSensors->setEnabled(checked);
    ui->groupTrigger->setEnabled(checked);
    ui->groupBluetooth->setEnabled(checked);

    if (checked) {
        psm->setRumble(ui->sliderRumble->value());
        psm->setColor(colorLEDs);
    } else {
        psm->setRumble(0);
        psm->setColor(Qt::black);
    }

    psm->setEnabled(checked);
}

void PSMoveTestGUI::setColor(QColor color)
{
    colorLEDs = color;
    ui->labelLEDs->setText("<div style='background-color: "+colorLEDs.name()+
                           "; color: "+colorLEDs.name()+";'>LEDs</div>");

    if (psm->enabled()) {
        psm->setColor(color);
    }
}

void PSMoveTestGUI::on_buttonLEDs_clicked()
{
    QColor result = QColorDialog::getColor(colorLEDs, this);

    if (result.isValid()) {
        setColor(result);
    }
}

void PSMoveTestGUI::on_buttonQuit_clicked()
{
    QApplication::quit();
}

void PSMoveTestGUI::setTrigger()
{
    ui->progressbarTrigger->setValue(psm->trigger());
}

void PSMoveTestGUI::on_sliderRumble_sliderMoved(int position)
{
    if (psm->enabled()) {
        psm->setRumble(position);
    }
}

void PSMoveTestGUI::onButtonPressed(int button)
{
    setButton(button, true);
}

void PSMoveTestGUI::onButtonReleased(int button)
{
    setButton(button, false);
}

void PSMoveTestGUI::setButton(int button, bool pressed)
{
    switch ((PSMoveQt::ButtonType)button) {
        case PSMoveQt::Start:
            ui->checkboxStart->setChecked(pressed);
            break;
        case PSMoveQt::Select:
            ui->checkboxSelect->setChecked(pressed);
            break;
        case PSMoveQt::T:
            ui->checkboxTrigger->setChecked(pressed);
            break;
        case PSMoveQt::Move:
            ui->checkboxMove->setChecked(pressed);
            break;
        case PSMoveQt::Square:
            ui->checkboxSquare->setChecked(pressed);
            break;
        case PSMoveQt::Triangle:
            ui->checkboxTriangle->setChecked(pressed);
            break;
        case PSMoveQt::Cross:
            ui->checkboxCross->setChecked(pressed);
            break;
        case PSMoveQt::Circle:
            ui->checkboxCircle->setChecked(pressed);
            break;
        case PSMoveQt::PS:
            ui->checkboxPS->setChecked(pressed);
            break;
        default:
            break;
    }
}

void PSMoveTestGUI::on_buttonBluetoothGet_clicked()
{
    ui->lineeditBluetooth->setText("FIXME");
}

void PSMoveTestGUI::on_buttonBluetoothSet_clicked()
{
    QMessageBox::information(this, "FIXME", "FIXME");
}
