DEFINES += USE_OPENCV # Comment out to disable OpenCV. You'll have no filtering or scaler, but you also don't need to provide the dependencies.
DEFINES += USE_RGBEASY_API
DEFINES += QT_NO_DEBUG_OUTPUT
DEFINES += VCS_ON_QT
#DEFINES += VALIDATION_RUN

# For now, disable the RGBEASY API for validating, just to simplify things. Once the
# validatiom system is a bit better fleshed out, this should not be needed.
contains(DEFINES, VALIDATION_RUN) {
    DEFINES -= USE_RGBEASY_API
}

# Specific configurations for the current platform.
linux {
    DEFINES -= USE_RGBEASY_API  # By default, the RGBEASY API isn't available on Linux (not with my capture card, anyway).

    # OpenCV.
    contains(DEFINES, USE_OPENCV) {
        LIBS += -lopencv_core -lopencv_imgproc
    }
}
win32 {
    # The RGBEASY API.
    contains(DEFINES, USE_RGBEASY_API) {
        INCLUDEPATH += "C:/Program Files (x86)/Vision RGB-PRO/SDK/RGBEASY/INCLUDE"
        LIBS += "C:/Program Files (x86)/Vision RGB-PRO/SDK/RGBEASY/LIB/RGBEASY.lib"
    }

    # OpenCV.
    contains(DEFINES, USE_OPENCV) {
        INCLUDEPATH += "C:/Program Files (x86)/OpenCV/3.2.0/include"
        LIBS += -L"C:/Program Files (x86)/OpenCV/3.2.0/bin/mingw"
        LIBS += -lopencv_core320 -lopencv_imgproc320
    }
}

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = vcs
TEMPLATE = app
CONFIG += console c++11

OBJECTS_DIR = generated_files
MOC_DIR = generated_files
UI_DIR = generated_files

INCLUDEPATH += $$PWD/src/qt

SOURCES += \
    src/qt/d_window.cpp \
    src/qt/d_resolution_dialog.cpp \
    src/qt/d_main.cpp \
    src/qt/d_control_panel.cpp \
    src/qt/d_video_and_color_dialog.cpp \
    src/qt/d_overlay_dialog.cpp \
    src/qt/d_alias_dialog.cpp \
    src/qt/d_anti_tear_dialog.cpp \
    src/qt/dw_filter_list.cpp \
    src/qt/dw_filter_complex_list.cpp \
    src/scaler.cpp \
    src/main.cpp \
    src/log.cpp \
    src/filter.cpp \
    src/command_line.cpp \
    src/capture.cpp \
    src/anti_tear.cpp \
    src/persistent_settings.cpp \
    src/qt/d_filter_complex_dialog.cpp \
    src/tests/test_scaling.cpp \
    src/memory.cpp

HEADERS += \
    src/common.h \
    src/null_rgbeasy.h \
    src/types.h \
    src/qt/d_window.h \
    src/qt/d_resolution_dialog.h \
    src/scaler.h \
    src/capture.h \
    src/display.h \
    src/main.h \
    src/qt/d_control_panel.h \
    src/log.h \
    src/qt/d_video_and_color_dialog.h \
    src/qt/d_overlay_dialog.h \
    src/qt/d_alias_dialog.h \
    src/anti_tear.h \
    src/qt/d_anti_tear_dialog.h \
    src/qt/dw_filter_list.h \
    src/filter.h \
    src/qt/dw_filter_complex_list.h \
    src/command_line.h \
    src/qt/df_filters.h \
    src/tests/test_scaling.h \
    src/persistent_settings.h \
    src/qt/d_util.h \
    src/qt/d_filter_complex_dialog.h \
    src/csv.h \
    src/memory.h \
    src/memory_interface.h

FORMS += \
    src/qt/d_control_panel.ui \
    src/qt/d_window.ui \
    src/qt/d_video_and_color_dialog.ui \
    src/qt/d_overlay_dialog.ui \
    src/qt/d_resolution_dialog.ui \
    src/qt/d_alias_dialog.ui \
    src/qt/d_anti_tear_dialog.ui \
    src/qt/d_filter_complex_dialog.ui

# C++. For GCC/Clang/MinGW.
QMAKE_CXXFLAGS += -g
QMAKE_CXXFLAGS += -O2
QMAKE_CXXFLAGS += -Wall
QMAKE_CXXFLAGS += -ansi
QMAKE_CXXFLAGS += -pipe
QMAKE_CXXFLAGS += -pedantic
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -Wno-missing-field-initializers
