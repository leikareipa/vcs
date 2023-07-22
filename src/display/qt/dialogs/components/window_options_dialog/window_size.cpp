#include "display/qt/persistent_settings.h"
#include "common/vcs_event/vcs_event.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "window_size.h"
#include "ui_window_size.h"

WindowSize::WindowSize(QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::WindowSize)
{
    ui->setupUi(this);

    this->set_name("Window size");

    disable_output_size_controls(false);

    // Initialize GUI controls to their starting values.
    {
        ui->spinBox_outputResX->setMaximum(MAX_OUTPUT_WIDTH);
        ui->spinBox_outputResX->setMinimum(MIN_OUTPUT_WIDTH);

        ui->spinBox_outputResY->setMaximum(MAX_OUTPUT_HEIGHT);
        ui->spinBox_outputResY->setMinimum(MIN_OUTPUT_HEIGHT);
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        // Returns true if the given checkbox is in a proper checked state. Returns
        // false if it's in a proper unchecked state; and assert fails if the checkbox
        // is neither fully checked nor fully unchecked.
        const auto is_checked = [](int qcheckboxCheckedState)->bool
        {
            k_assert(
                (qcheckboxCheckedState != Qt::PartiallyChecked),
                "Expected this QCheckBox to have a two-state toggle. It appears to have a third state."
            );

            return ((qcheckboxCheckedState == Qt::Checked)? true : false);
        };

        connect(ui->checkBox_forceOutputScale, &QCheckBox::stateChanged, this, [is_checked, this](int state)
        {
            if (is_checked(state))
            {
                ks_set_scaling_multiplier(ui->spinBox_outputScale->value() / 100.0);
            }
            else
            {
                this->ui->spinBox_outputScale->setValue(100);
            }

            ui->spinBox_outputScale->setEnabled(is_checked(state));
            kpers_set_value(INI_GROUP_OUTPUT_WINDOW, "ForceWindowScale", bool(state));

            ks_set_scaling_multiplier_enabled(is_checked(state));
        });

        connect(ui->checkBox_forceOutputRes, &QCheckBox::stateChanged, this, [is_checked, this](int state)
        {
            this->ui->spinBox_outputResX->setEnabled(is_checked(state));
            this->ui->spinBox_outputResY->setEnabled(is_checked(state));
            kpers_set_value(INI_GROUP_OUTPUT_WINDOW, "ForceWindowSize", bool(state));

            ks_set_base_resolution_enabled(is_checked(state));

            if (is_checked(state))
            {
                ks_set_base_resolution({
                    .w = unsigned(this->ui->spinBox_outputResX->value()),
                    .h = unsigned(this->ui->spinBox_outputResY->value())
                });
            }
            else
            {
                this->ui->spinBox_outputResX->setValue(ks_base_resolution().w);
                this->ui->spinBox_outputResY->setValue(ks_base_resolution().h);
            }
        });

        connect(ui->spinBox_outputResX, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this]
        {
            if (!ui->checkBox_forceOutputRes->isChecked()) return;

            const resolution_s r = {
                (uint)ui->spinBox_outputResX->value(),
                (uint)ui->spinBox_outputResY->value()
            };

            ks_set_base_resolution(r);
            kpers_set_value(INI_GROUP_OUTPUT_WINDOW, "WindowWidth", uint(r.w));
        });

        connect(ui->spinBox_outputResY, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this]
        {
            if (!ui->checkBox_forceOutputRes->isChecked()) return;

            const resolution_s r = {
                (uint)ui->spinBox_outputResX->value(),
                (uint)ui->spinBox_outputResY->value()
            };

            ks_set_base_resolution(r);
            kpers_set_value(INI_GROUP_OUTPUT_WINDOW, "WindowHeight", uint(r.h));
        });

        connect(ui->spinBox_outputScale, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this]
        {
            ks_set_scaling_multiplier(ui->spinBox_outputScale->value() / 100.0);
            kpers_set_value(INI_GROUP_OUTPUT_WINDOW, "WindowScale", ui->spinBox_outputScale->value());
        });
    }

    // Restore persistent settings.
    {
        ui->checkBox_forceOutputRes->setChecked(kpers_value_of(INI_GROUP_OUTPUT_WINDOW, "ForceWindowSize", false).toBool());
        ui->spinBox_outputResX->setValue(kpers_value_of(INI_GROUP_OUTPUT_WINDOW, "WindowWidth", 640).toUInt());
        ui->spinBox_outputResY->setValue(kpers_value_of(INI_GROUP_OUTPUT_WINDOW, "WindowHeight", 480).toUInt());
        ui->checkBox_forceOutputScale->setChecked(kpers_value_of(INI_GROUP_OUTPUT_WINDOW, "ForceWindowScale", false).toBool());
        ui->spinBox_outputScale->setValue(kpers_value_of(INI_GROUP_OUTPUT_WINDOW, "WindowScale", 100).toInt());
    }

    // Listen for app events.
    {
        ev_new_video_mode.listen([this](const video_mode_s &videoMode)
        {
            if (!ui->checkBox_forceOutputRes->isChecked())
            {
                ui->spinBox_outputResX->setValue(videoMode.resolution.w);
                ui->spinBox_outputResY->setValue(videoMode.resolution.h);
            }
        });

        ev_custom_output_scaler_enabled.listen([this]
        {
            // The output scaling filter handles output sizing, so we want to bar the
            // user from manipulating it via this dialog.
            this->disable_output_size_controls(true);
        });

        ev_custom_output_scaler_disabled.listen([this]
        {
            this->disable_output_size_controls(false);
        });
    }

    return;
}

WindowSize::~WindowSize()
{
    delete ui;

    return;
}

// Adjusts the output scale value in the GUI by a pre-set step size in the given
// direction. Note that this will automatically trigger a change in the actual
// scaler output size as well.
void WindowSize::adjust_output_scaling(const int dir)
{
    const int curValue = ui->spinBox_outputScale->value();
    const int stepSize = ui->spinBox_outputScale->singleStep();

    k_assert((dir == 1 || dir == -1),
             "Expected the parameter for AdjustOutputScaleValue to be either 1 or -1.");

    if (!ui->checkBox_forceOutputScale->isChecked())
    {
        ui->checkBox_forceOutputScale->setChecked(true);
    }

    ui->spinBox_outputScale->setValue(curValue + (stepSize * dir));

    return;
}

void WindowSize::disable_output_size_controls(const bool areDisabled)
{
    ui->groupBox->setDisabled(areDisabled);

    return;
}

void WindowSize::notify_of_new_capture_signal(void)
{
    if (!ui->checkBox_forceOutputRes->isChecked())
    {
        const resolution_s r = ks_output_resolution();
        ui->spinBox_outputResX->setValue(r.w);
        ui->spinBox_outputResY->setValue(r.h);
    }

    return;
}
