/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "common/globals.h"
#include "display/qt/dialogs/linux_device_selector_dialog.h"
#include "ui_linux_device_selector_dialog.h"

LinuxDeviceSelectorDialog::LinuxDeviceSelectorDialog(unsigned *const deviceIdx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LinuxDeviceSelectorDialog),
    deviceIdx(deviceIdx)
{
    ui->setupUi(this);

    this->setWindowTitle("Select an input channel");

    ui->spinBox_deviceIdx->setValue(*deviceIdx);
    ui->spinBox_deviceIdx->setFocus();

    // Connect GUI controls to consequences for operating them.
    {
        connect(ui->pushButton_ok, &QPushButton::clicked, this, [=]
        {
            k_assert((ui->spinBox_deviceIdx->value() >= 0), "The device index must be a positive value.");

            *deviceIdx = unsigned(ui->spinBox_deviceIdx->value());

            this->accept();
        });

        connect(ui->pushButton_cancel, &QPushButton::clicked, this, [=]
        {
            this->reject();
        });
    }

    return;
}

LinuxDeviceSelectorDialog::~LinuxDeviceSelectorDialog()
{
    delete ui;

    return;
}
