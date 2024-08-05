#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_RENDERER_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_RENDERER_H

#include "display/qt/widgets/DialogFragment.h"

namespace control_panel::output
{
    namespace Ui { class Window; }

    class Window : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit Window(QWidget *parent = nullptr);
        ~Window();

        std::string fullscreen_aspect_ratio(void);

    private:
        Ui::Window *ui;
    };
}

#endif
