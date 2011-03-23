#ifndef DEVICECHOOSERDIALOG_H
#define DEVICECHOOSERDIALOG_H

#include <QDialog>

#include <psmovetestgui.h>

namespace Ui {
    class DeviceChooserDialog;
}

class DeviceChooserDialog : public QDialog
{
    Q_OBJECT
    PSMoveTestGUI *testgui;

public:
    explicit DeviceChooserDialog(PSMoveTestGUI *parent = 0);
    ~DeviceChooserDialog();

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Ui::DeviceChooserDialog *ui;
};

#endif // DEVICECHOOSERDIALOG_H
