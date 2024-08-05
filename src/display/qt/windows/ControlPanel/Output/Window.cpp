#include <QWindow>
#include "display/display.h"
#include "display/qt/windows/OutputWindow.h"
#include "display/qt/persistent_settings.h"
#include "Window.h"
#include "ui_Window.h"

control_panel::output::Window::Window(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::Window)
{
    ui->setupUi(this);

    connect(ui->comboBox_renderer, &QComboBox::currentTextChanged, this, [parent](const QString &rendererName)
    {
        kpers_set_value(INI_GROUP_OUTPUT_WINDOW, "Renderer", rendererName);
        OutputWindow::current_instance()->set_opengl_enabled(rendererName == "OpenGL");
    });

    connect(ui->comboBox_fullscreenAspectRatio, &QComboBox::currentTextChanged, this, [parent](const QString &ratio)
    {
        kpers_set_value(INI_GROUP_OUTPUT_WINDOW, "FullscreenAspectRatio", ratio);
    });

    connect(ui->lineEdit_title, &QLineEdit::textChanged, this, [parent](const QString &title)
    {
        kpers_set_value(INI_GROUP_OUTPUT_WINDOW, "Title", title);
        OutputWindow::current_instance()->override_window_title(title);
    });

    ui->comboBox_fullscreenAspectRatio->setCurrentText(kpers_value_of(INI_GROUP_OUTPUT_WINDOW, "FullscreenAspectRatio", "Source ratio").toString());
    ui->comboBox_renderer->setCurrentText(kpers_value_of(INI_GROUP_OUTPUT_WINDOW, "Renderer", ui->comboBox_renderer->itemText(0)).toString());
    ui->lineEdit_title->setText(kpers_value_of(INI_GROUP_OUTPUT_WINDOW, "Title", ui->lineEdit_title->text()).toString());
}

control_panel::output::Window::~Window()
{
    delete ui;
}

std::string control_panel::output::Window::fullscreen_aspect_ratio()
{
    return ui->comboBox_fullscreenAspectRatio->currentText().toStdString();
}
