#include "devicechooserdialog.h"
#include "ui_devicechooserdialog.h"

DeviceChooserDialog::DeviceChooserDialog(PSMoveTestGUI *parent) :
    QDialog(parent),
    testgui(parent),
    ui(new Ui::DeviceChooserDialog)
{
    ui->setupUi(this);
}

DeviceChooserDialog::~DeviceChooserDialog()
{
    delete ui;
}

void DeviceChooserDialog::on_buttonBox_accepted()
{
    int index = ui->comboBox->currentIndex();
    destroy();

    testgui->reconnectByIndex(index);
}

void DeviceChooserDialog::on_buttonBox_rejected()
{
    destroy();
    testgui->reconnectByIndex(-1);
}
