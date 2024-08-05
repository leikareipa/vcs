#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_SIZE_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_SIZE_H

#include "display/qt/widgets/DialogFragment.h"

struct resolution_s;

namespace control_panel::output
{
    namespace Ui { class Size; }

    class Size : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit Size(QWidget *parent = 0);
        ~Size();

        void adjust_output_scaling(const int dir);

        void disable_output_size_controls(const bool areDisabled);

        void set_output_size(const resolution_s &newSize);

        void set_output_scale(const unsigned newScale);

        resolution_s get_output_size(void);

        unsigned get_output_scale(void);

        void set_output_size_enabled(const bool is);

        void set_output_scale_enabled(const bool is);

        bool is_output_size_enabled(void);

        bool is_output_scale_enabled(void);

    private:
        Ui::Size *ui;
    };
}

#endif
