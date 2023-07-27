/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * Uses Qt's OpenGL implementation to draw the contents of the VCS frame buffer
 * to screen. I.e. just draws a full-window textured quad.
 *
 */

#include <QCoreApplication>
#include <QOpenGLWidget>
#include <QMatrix4x4>
#include "display/qt/widgets/QOpenGLWidget_opengl_renderer.h"
#include "capture/capture.h"
#include "display/display.h"
#include "common/globals.h"
#include "scaler/scaler.h"

// The texture into which we'll stream the captured frames.
GLuint FRAMEBUFFER_TEXTURE;

// The texture in which we'll display the current output overlay, if any.
GLuint OVERLAY_TEXTURE;

// A function that returns the current overlay as a QImage.
std::function<QImage()> OVERLAY_AS_QIMAGE_F;

OGLWidget::OGLWidget(std::function<QImage()> overlay_as_qimage, QWidget *parent) : QOpenGLWidget(parent)
{
    OVERLAY_AS_QIMAGE_F = overlay_as_qimage;

    // Needed for auto-hiding the parent window's menu bar to work.
    this->setMouseTracking(true);

    return;
}

void OGLWidget::initializeGL()
{
    this->initializeOpenGLFunctions();

    DEBUG(("OpenGL is reported to be version %s.", glGetString(GL_VERSION)));

    this->glDisable(GL_DEPTH_TEST);
    this->glEnable(GL_TEXTURE_2D);

    this->glGenTextures(1, &FRAMEBUFFER_TEXTURE);
    this->glBindTexture(GL_TEXTURE_2D, FRAMEBUFFER_TEXTURE);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    this->glGenTextures(1, &OVERLAY_TEXTURE);
    this->glBindTexture(GL_TEXTURE_2D, OVERLAY_TEXTURE);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // For alpha-blending the overlay image.
    this->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    this->glEnable(GL_BLEND);

    k_assert(!this->glGetError(), "OpenGL initialization failed.");

    return;
}

void OGLWidget::resizeGL(int w, int h)
{
    QMatrix4x4 m;
    m.setToIdentity();
    m.ortho(0, w, h, 0, -1, 1);

    this->glLoadMatrixf(m.constData());

    return;
}

void OGLWidget::paintGL()
{
    // Draw the output frame.
    const image_s outputImage = ks_scaler_frame_buffer();
    if (outputImage.pixels)
    {
        this->glDisable(GL_BLEND);

        this->glBindTexture(GL_TEXTURE_2D, FRAMEBUFFER_TEXTURE);
        this->glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            outputImage.resolution.w,
            outputImage.resolution.h,
            0,
            GL_BGRA,
            GL_UNSIGNED_BYTE,
            outputImage.pixels
        );

        glBegin(GL_TRIANGLES);
            glTexCoord2i(0, 0); glVertex2i(0,             0);
            glTexCoord2i(0, 1); glVertex2i(0,             this->height());
            glTexCoord2i(1, 1); glVertex2i(this->width(), this->height());

            glTexCoord2i(1, 1); glVertex2i(this->width(), this->height());
            glTexCoord2i(1, 0); glVertex2i(this->width(), 0);
            glTexCoord2i(0, 0); glVertex2i(0,             0);
        glEnd();
    }

    // Draw the overlay, if any.
    const QImage overlay = OVERLAY_AS_QIMAGE_F();
    if (!overlay.isNull())
    {
        this->glEnable(GL_BLEND);

        this->glBindTexture(GL_TEXTURE_2D, OVERLAY_TEXTURE);
        this->glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            overlay.width(),
            overlay.height(),
            0,
            GL_BGRA,
            GL_UNSIGNED_BYTE,
            overlay.constBits()
        );

        glBegin(GL_TRIANGLES);
            glTexCoord2i(0, 0); glVertex2i(0,             0);
            glTexCoord2i(0, 1); glVertex2i(0,             this->height());
            glTexCoord2i(1, 1); glVertex2i(this->width(), this->height());

            glTexCoord2i(1, 1); glVertex2i(this->width(), this->height());
            glTexCoord2i(1, 0); glVertex2i(this->width(), 0);
            glTexCoord2i(0, 0); glVertex2i(0,             0);
        glEnd();
    }

    this->glFlush();

    return;
}
