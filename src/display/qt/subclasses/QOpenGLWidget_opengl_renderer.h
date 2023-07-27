/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QOPENGLWIDGET_OPENGL_RENDERER_H
#define VCS_DISPLAY_QT_SUBCLASSES_QOPENGLWIDGET_OPENGL_RENDERER_H

#include <QOpenGLFunctions_1_2>
#include <QOpenGLWidget>
#include <QWidget>
#include <functional>
#include "common/types.h"

class Overlay;

class OGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_1_2
{
    Q_OBJECT

public:
    explicit OGLWidget(std::function<QImage()> overlay_as_qimage, QWidget *parent = 0);

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
};

#endif
