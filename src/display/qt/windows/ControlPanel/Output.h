#ifndef VCS_DISPLAY_QT_DIALOGS_WINDOW_OPTIONS_DIALOGH
#define VCS_DISPLAY_QT_DIALOGS_WINDOW_OPTIONS_DIALOGH

#include "display/qt/widgets/DialogFragment.h"

namespace control_panel::output
{
    class Size;
    class Scaler;
    class Overlay;
    class Window;
    class Histogram;
    class Status;
}

namespace control_panel
{
    namespace Ui { class Output; }

    class Output : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit Output(QWidget *parent = nullptr);
        ~Output();

        control_panel::output::Size* size(void) const { return this->sizeDialog; }
        control_panel::output::Window* window(void) const { return this->windowDialog; }
        control_panel::output::Overlay* overlay(void) const { return this->overlayDialog; }

    private:
        Ui::Output *ui;

        control_panel::output::Size *sizeDialog = nullptr;
        control_panel::output::Scaler *scalerDialog = nullptr;
        control_panel::output::Overlay *overlayDialog = nullptr;
        control_panel::output::Window *windowDialog = nullptr;
        control_panel::output::Histogram *histogramDialog = nullptr;
        control_panel::output::Status *outputStatusDialog = nullptr;
    };
}

#endif
