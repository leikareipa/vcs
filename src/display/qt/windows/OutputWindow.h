/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_WINDOWS_OUTPUT_WINDOW_H
#define VCS_DISPLAY_QT_WINDOWS_OUTPUT_WINDOW_H

#include <QMainWindow>
#include <QApplication>
#include "display/display.h"
#include "common/globals.h"
#include "common/assert.h"

class ControlPanel;
class MagnifyingGlass;

namespace Ui {
class OutputWindow;
}

class OutputWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit OutputWindow(QWidget *parent = 0);
    ~OutputWindow();

    // Gets user input in the GUI, etc.
    void update_gui_state(void);

    // Returns true if the window has a border.
    bool window_has_border(void);

    void add_gui_log_entry(const log_entry_s e);

    void update_window_size(void);

    // Updates the window title with the current status of capture and that of the program
    // in general.
    void update_window_title(void);

    void set_opengl_enabled(const bool enabled);

    void redraw(void);

    // Returns the current overlay as a QImage, or a null QImage if the overlay
    // should not be shown at this time.
    QImage overlay_image(void);

    bool apply_global_stylesheet(void);

    bool set_global_font_size(const unsigned fontSize);

    bool is_fullscreen(void);
    std::string fullscreen_ratio(void);
    resolution_s max_fullscreen_size(void);

    void override_window_title(const QString &newTitle);

    ControlPanel* control_panel(void) const { return this->controlPanelWindow; }

    void add_control_panel_widget(const std::string &tabName, const std::string &widgetTitle, const abstract_gui_s &widget);

    static OutputWindow* current_instance(void)
    {
        for (QWidget *const w: qApp->topLevelWidgets())
        {
            if (qobject_cast<OutputWindow*>(w))
            {
                return qobject_cast<OutputWindow*>(w);
            }
        }

        k_assert(0, "No instance of the output window is available.");
        return nullptr;
    }
signals:
    void fullscreen_mode_enabled(void);
    void fullscreen_mode_disabled(void);
    void border_hidden(void);
    void border_shown(void);

private:
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void changeEvent(QEvent *event);
    void paintEvent(QPaintEvent *);
    void closeEvent(QCloseEvent *event);
    void wheelEvent(QWheelEvent *event);

    // Load the font pointed to by the given filename, and make it available to the program.
    bool load_font(const QString &filename);

    void save_screenshot(void);

    void set_fullscreen(const bool is);

    Ui::OutputWindow *ui = nullptr;

    // The menu items shown when the user right-clicks this window.
    std::vector<QMenu*> contextMenuSubmenus;
    QMenu *contextMenu;

    ControlPanel *controlPanelWindow = nullptr;

    MagnifyingGlass *magnifyingGlass = nullptr;

    // Set to true when the user has selected to close the
    // main window.
    bool userExiting = false;

    // Set to true if the program failed to initialize the capture card.
    bool captureInitFail = false;

    // A user-definable custom title for the output window.
    QString windowTitleOverride = "";

    // The currently-active app-wide QSS style sheet.
    QString appwideStyleSheet = "";

    unsigned appwideFontSize = 15;

    unsigned frameDropCount = false;

    const Qt::WindowFlags defaultWindowFlags = (Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
};

#endif
