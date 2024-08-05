/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS display wrapper
 *
 * Implements VCS's display interface using Qt, i.e. wraps the interface's
 * functions around the Qt-based GUI.
 *
 */

#include <QApplication>
#include <QMessageBox>
#include <cassert>
#include <thread>
#include "display/qt/windows/OutputWindow.h"
#include "display/qt/windows/ControlPanel/VideoPresets.h"
#include "display/qt/windows/ControlPanel/FilterGraph.h"
#include "display/qt/windows/ControlPanel.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "filter/filter.h"
#include "common/log/log.h"

// We'll want to avoid accessing the GUI via non-GUI threads, so let's assume
// the thread that creates this unit is the GUI thread. We'll later compare
// against this id to detect out-of-GUI-thread access.
static const std::thread::id NATIVE_THREAD_ID = std::this_thread::get_id();

struct custom_widget_queue_item_s
{
    std::string tabName;
    std::string widgetTitle;
    abstract_gui_s *widget;
};

static std::vector<custom_widget_queue_item_s> CUSTOM_WIDGET_QUEUE;

// Qt wants a QApplication object around for the GUI to function.
namespace app_n
{
    static int ARGC = 1;
    static char NAME[] = "VCS";
    static char *ARGV = NAME;
    static QApplication *const APP = new QApplication(ARGC, &ARGV);
}

// The window we'll display the program in. Also owns the various sub-dialogs, etc.
static OutputWindow *WINDOW = nullptr;

#define ASSERT_WINDOW_IS_NOT_NULL k_assert(WINDOW, "Tried to query the display before it had been initialized.");

void kd_recalculate_filter_graph_chains(void)
{
    ASSERT_WINDOW_IS_NOT_NULL;
    WINDOW->control_panel()->filter_graph()->recalculate_filter_chains();

    return;
}

subsystem_releaser_t kd_acquire_output_window(void)
{
    DEBUG(("Acquiring the display."));

    WINDOW = new OutputWindow;

    for (const auto t: CUSTOM_WIDGET_QUEUE)
    {
        WINDOW->add_control_panel_widget(t.tabName, t.widgetTitle, *t.widget);
    }

    WINDOW->show();

    return []{
        DEBUG(("Releasing the display."));
        if (WINDOW)
        {
            delete WINDOW;
        }
    };
}

void kd_spin_event_loop(void)
{
    ASSERT_WINDOW_IS_NOT_NULL;
    WINDOW->update_gui_state();

    return;
}

void kd_update_output_window_title(void)
{
    if (WINDOW)
    {
        WINDOW->update_window_title();
    }

    return;
}

void kd_load_video_presets(const std::string &filename)
{
    ASSERT_WINDOW_IS_NOT_NULL;
    WINDOW->control_panel()->video_presets()->load_presets_from_file(QString::fromStdString(filename));
}

void kd_load_filter_graph(const std::string &filename)
{
    ASSERT_WINDOW_IS_NOT_NULL;
    WINDOW->control_panel()->filter_graph()->load_graph_from_file(QString::fromStdString(filename));
}

bool kd_is_fullscreen(void)
{
    if (!WINDOW)
    {
        return false;
    }

    return WINDOW->is_fullscreen();
}

std::string kd_fullscreen_ratio(void)
{
    ASSERT_WINDOW_IS_NOT_NULL;
    return WINDOW->fullscreen_ratio();
}

resolution_s kd_max_fullscreen_size(void)
{
    ASSERT_WINDOW_IS_NOT_NULL;
    return WINDOW->max_fullscreen_size();
}

void kd_show_headless_info_message(const char *const title, const char *const msg)
{
    if (std::this_thread::get_id() == NATIVE_THREAD_ID)
    {
        QMessageBox mb;
        mb.setWindowTitle(strlen(title) == 0? "VCS has this to say" : title);
        mb.setText(msg);
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setIcon(QMessageBox::Information);
        mb.setDefaultButton(QMessageBox::Ok);

        mb.exec();
    }

    INFO(("%s", msg));

    return;
}

// Displays the given error in a message box that isn't tied to a particular window
// of the program. Useful for giving out e.g. startup error messages for things that
// occur before the GUI has been initialized.
//
void kd_show_headless_error_message(const char *const title, const char *const msg)
{
    if (std::this_thread::get_id() == NATIVE_THREAD_ID)
    {
        QMessageBox mb;
        mb.setWindowTitle(strlen(title) == 0? "VCS has this to say" : title);
        mb.setText(msg);
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setIcon(QMessageBox::Critical);
        mb.setDefaultButton(QMessageBox::Ok);

        mb.exec();
    }

    NBENE(("%s", msg));

    return;
}
void kd_show_headless_assert_error_message(const char *const msg, const char *const filename, const uint lineNum)
{
    if (std::this_thread::get_id() == NATIVE_THREAD_ID)
    {
        QMessageBox mb;
        mb.setWindowTitle("Assertion failure");
        mb.setText(
            "<html>"
            "VCS has come across an unexpected condition in its code and as a precaution will now exit.<br><br>"
            "\"" + QString(msg) + "\"<br>"
            "<b>In file:</b> " + QString(filename) + "<br>"
            "<b>On line:</b> " + QString::number(lineNum) +
            "</html>"
        );
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setIcon(QMessageBox::Critical);
        mb.setDefaultButton(QMessageBox::Ok);

        mb.exec();
    }

    return;
}

void kd_add_control_panel_widget(const std::string &tabName, const std::string &widgetTitle, abstract_gui_s *widget)
{
    // We'll queue the widget for adding when the display has been acquired,
    // to allow subsystem initializers that run before the display initializer
    // to add custom widgets.
    CUSTOM_WIDGET_QUEUE.push_back({
        .tabName = tabName,
        .widgetTitle = widgetTitle,
        .widget = widget,
    });

    return;
}
