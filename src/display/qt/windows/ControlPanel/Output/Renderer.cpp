#include <QWindow>
#include "display/display.h"
#include "display/qt/windows/OutputWindow.h"
#include "display/qt/persistent_settings.h"
#include "Renderer.h"
#include "ui_Renderer.h"

control_panel::output::Renderer::Renderer(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::Renderer)
{
    ui->setupUi(this);

    connect(ui->comboBox_renderer, &QComboBox::currentTextChanged, this, [parent](const QString &rendererName)
    {
        kpers_set_value(INI_GROUP_OUTPUT_WINDOW, "Renderer", rendererName);

        for (QWidget *const w: qApp->topLevelWidgets())
        {
            const auto outputWindow = qobject_cast<OutputWindow*>(w);
            if (outputWindow)
            {
                outputWindow->set_opengl_enabled(rendererName == "OpenGL");
                return;
            }
        }

        NBENE(("Failed to set the renderer: Could not find the output window."));
    });

    // Restore the previous persistent setting.
    ui->comboBox_renderer->setCurrentText(kpers_value_of(INI_GROUP_OUTPUT_WINDOW, "Renderer", ui->comboBox_renderer->itemText(0)).toString());
}

control_panel::output::Renderer::~Renderer()
{
    delete ui;
}
