
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
