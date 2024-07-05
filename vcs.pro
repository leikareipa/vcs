#
# VCS
# 2017-2024 Tarpeeksi Hyvae Soft
#

#..............................................................................
#
# Customizable build parameters.
#
#..............................................................................

# Path to the Datapath Linux Vision driver header files. Needed only if
# CAPTURE_BACKEND_VISION_V4L is defined.
DATAPATH_VISION_HEADER_PATH = $$(HOME)/sdk/visionrgb/include/

# Paths to OpenCV's header and library files. The debug libraries are used only
# if VCS_DEBUG_BUILD is defined.
OPENCV_HEADER_PATH = /usr/include/opencv4/
OPENCV_LIB_PATH = /usr/lib/x86_64-linux-gnu/
OPENCV_DEBUG_LIB_PATH = /usr/lib/debug/opencv4/

# Whether to produce a non-release-mode debugging build of VCS, which includes
# additional run-time error checks etc. Optionally, you can enable sanitizers
# in debug builds.
#
# Note: if a debug build uses _GLIBCXX_DEBUG or sanitizers, you may (or might
# not) come across incompatibilities with OpenCV.
DEFINES += \
    #VCS_DEBUG_BUILD \
    #VCS_DEBUG_BUILD_USES_SANITIZERS

# Which capture source this build supports. Select one. See the build guide in
# README.md for more information.
DEFINES += \
    CAPTURE_BACKEND_VISION_V4L
    #CAPTURE_BACKEND_VIRTUAL
    #CAPTURE_BACKEND_GPHOTO2
    #CAPTURE_BACKEND_MMAP
    #CAPTURE_BACKEND_GENERIC_V4L

# Whether this build of VCS uses the OpenCV library. For now, non-OpenCV builds
# are not supported, so this should always be defined.
DEFINES += VCS_USES_OPENCV

#..............................................................................
#
# Stuff you mostly don't need to care about.
#
#..............................................................................

QMAKE_CC = gcc
QMAKE_CXX = g++

QMAKE_CXXFLAGS += \
    -pedantic \
    -std=c++2a \
    -Wno-missing-field-initializers

INCLUDEPATH += \
    $$PWD/src/ \
    $$PWD/src/display/qt/widgets/

SOURCES += \
    src/capture/alias.cpp \
    src/common/disk/file_readers/file_reader_video_presets_version_b.cpp \
    src/common/disk/file_writers/file_writer_video_presets_version_b.cpp \
    src/common/refresh_rate.cpp \
    src/common/vcs_event/vcs_event.cpp \
    src/display/qt/wheel_blocker.cpp \
    src/display/qt/widgets/ColorHistogram.cpp \
    src/display/qt/widgets/DialogFileMenu.cpp \
    src/display/qt/widgets/DialogFragment.cpp \
    src/display/qt/widgets/HorizontalSlider.cpp \
    src/display/qt/widgets/InteractibleNodeGraph.cpp \
    src/display/qt/widgets/InteractibleNodeGraphNode.cpp \
    src/display/qt/widgets/InteractibleNodeGraphView.cpp \
    src/display/qt/widgets/MagnifyingGlass.cpp \
    src/display/qt/widgets/OGLWidget.cpp \
    src/display/qt/widgets/ParameterGrid.cpp \
    src/display/qt/widgets/PropertyTable.cpp \
    src/display/qt/widgets/QtAbstractGUI.cpp \
    src/display/qt/widgets/ResolutionQuery.cpp \
    src/display/qt/widgets/VideoPresetList.cpp \
    src/display/qt/windows/ControlPanel.cpp \
    src/display/qt/windows/ControlPanel/Capture/AliasResolutions.cpp \
    src/display/qt/windows/ControlPanel/Capture/CaptureRate.cpp \
    src/display/qt/windows/ControlPanel/Capture/InputChannel.cpp \
    src/display/qt/windows/ControlPanel/Capture/InputResolution.cpp \
    src/display/qt/windows/ControlPanel/Capture/SignalStatus.cpp \
    src/display/qt/windows/ControlPanel/FilterGraph/BaseFilterGraphNode.cpp \
    src/display/qt/windows/ControlPanel/FilterGraph/FilterNode.cpp \
    src/display/qt/windows/ControlPanel/FilterGraph/InputGateNode.cpp \
    src/display/qt/windows/ControlPanel/FilterGraph/OutputGateNode.cpp \
    src/display/qt/windows/ControlPanel/FilterGraph/OutputScalerNode.cpp \
    src/display/qt/windows/ControlPanel/Output/Histogram.cpp \
    src/display/qt/windows/ControlPanel/Output/Overlay.cpp \
    src/display/qt/windows/ControlPanel/Output/Scaler.cpp \
    src/display/qt/windows/ControlPanel/Output/Size.cpp \
    src/display/qt/windows/ControlPanel/Output/Status.cpp \
    src/display/qt/windows/ControlPanel/Output/Window.cpp \
    src/display/qt/windows/OutputWindow.cpp \
    src/display/qt/windows/ControlPanel/AboutVCS.cpp \
    src/display/qt/windows/ControlPanel/Capture.cpp \
    src/display/qt/windows/ControlPanel/FilterGraph.cpp \
    src/display/qt/windows/ControlPanel/Output.cpp \
    src/display/qt/windows/ControlPanel/VideoPresets.cpp \
    src/display/qt/keyboard_shortcuts.cpp \
    src/filter/filters/crt/filter_crt.cpp \
    src/filter/filters/output_scaler/filter_output_scaler.cpp \
    src/filter/filters/output_scaler/gui/filtergui_output_scaler.cpp \
    src/filter/filters/render_text/filter_render_text.cpp \
    src/filter/filters/render_text/gui/filtergui_render_text.cpp \
    src/main.cpp \
    src/display/display.cpp \
    src/display/qt/d_main.cpp \
    src/anti_tear/anti_tear_multiple_per_frame.cpp \
    src/anti_tear/anti_tear_one_per_frame.cpp \
    src/anti_tear/anti_tearer.cpp \
    src/filter/abstract_filter.cpp \
    src/filter/filters/input_gate/filter_input_gate.cpp \
    src/filter/filters/input_gate/gui/filtergui_input_gate.cpp \
    src/filter/filters/output_gate/filter_output_gate.cpp \
    src/filter/filters/output_gate/gui/filtergui_output_gate.cpp \
    src/scaler/scaler.cpp \
    src/common/log/log.cpp \
    src/filter/filter.cpp \
    src/common/command_line/command_line.cpp \
    src/capture/capture.cpp \
    src/display/qt/persistent_settings.cpp \
    src/common/disk/disk.cpp \
    src/common/disk/file_writers/file_writer_filter_graph_version_b.cpp \
    src/common/disk/file_reader.cpp \
    src/common/disk/file_readers/file_reader_filter_graph_version_b.cpp \
    src/common/disk/file_readers/file_reader_filter_graph_version_a.cpp \
    src/common/disk/file_writer.cpp \
    src/capture/video_presets.cpp \
    src/common/disk/file_readers/file_reader_video_presets_version_a.cpp \
    src/common/timer/timer.cpp

HEADERS += \
    src/capture/alias.h \
    src/common/assert.h \
    src/common/globals.h \
    src/display/qt/wheel_blocker.h \
    src/display/qt/widgets/ColorHistogram.h \
    src/display/qt/widgets/DialogFileMenu.h \
    src/display/qt/widgets/DialogFragment.h \
    src/display/qt/widgets/HorizontalSlider.h \
    src/display/qt/widgets/InteractibleNodeGraph.h \
    src/display/qt/widgets/InteractibleNodeGraphNode.h \
    src/display/qt/widgets/InteractibleNodeGraphView.h \
    src/display/qt/widgets/MagnifyingGlass.h \
    src/display/qt/widgets/OGLWidget.h \
    src/display/qt/widgets/ParameterGrid.h \
    src/display/qt/widgets/PropertyTable.h \
    src/display/qt/widgets/QtAbstractGUI.h \
    src/display/qt/widgets/ResolutionQuery.h \
    src/display/qt/widgets/VideoPresetList.h \
    src/display/qt/windows/ControlPanel.h \
    src/display/qt/windows/ControlPanel/Capture/AliasResolutions.h \
    src/display/qt/windows/ControlPanel/Capture/CaptureRate.h \
    src/display/qt/windows/ControlPanel/Capture/InputChannel.h \
    src/display/qt/windows/ControlPanel/Capture/InputResolution.h \
    src/display/qt/windows/ControlPanel/Capture/SignalStatus.h \
    src/display/qt/windows/ControlPanel/FilterGraph/BaseFilterGraphNode.h \
    src/display/qt/windows/ControlPanel/FilterGraph/FilterNode.h \
    src/display/qt/windows/ControlPanel/FilterGraph/InputGateNode.h \
    src/display/qt/windows/ControlPanel/FilterGraph/OutputGateNode.h \
    src/display/qt/windows/ControlPanel/FilterGraph/OutputScalerNode.h \
    src/display/qt/windows/ControlPanel/Output/Histogram.h \
    src/display/qt/windows/ControlPanel/Output/Overlay.h \
    src/display/qt/windows/ControlPanel/Output/Scaler.h \
    src/display/qt/windows/ControlPanel/Output/Size.h \
    src/display/qt/windows/ControlPanel/Output/Status.h \
    src/display/qt/windows/ControlPanel/Output/Window.h \
    src/display/qt/windows/OutputWindow.h \
    src/display/qt/windows/ControlPanel/AboutVCS.h \
    src/display/qt/windows/ControlPanel/Capture.h \
    src/display/qt/windows/ControlPanel/FilterGraph.h \
    src/display/qt/windows/ControlPanel/Output.h \
    src/display/qt/windows/ControlPanel/VideoPresets.h \
    src/display/qt/keyboard_shortcuts.h \
    src/anti_tear/anti_tear_multiple_per_frame.h \
    src/anti_tear/anti_tear_one_per_frame.h \
    src/anti_tear/anti_tearer.h \
    src/filter/filters/anti_tear/filter_anti_tear.h \
    src/filter/filters/blur/filter_blur.h \
    src/filter/filters/color_depth/filter_color_depth.h \
    src/filter/filters/crop/filter_crop.h \
    src/filter/filters/crt/filter_crt.h \
    src/filter/filters/decimate/filter_decimate.h \
    src/filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.h \
    src/filter/filters/filters.h \
    src/filter/filters/flip/filter_flip.h \
    src/filter/filters/focus_peaking/filter_focus_peaking.h \
    src/filter/filters/line_copy/filter_line_copy.h \
    src/filter/filters/output_scaler/filter_output_scaler.h \
    src/filter/filters/output_scaler/gui/filtergui_output_scaler.h \
    src/filter/filters/render_text/font.h \
    src/filter/filters/render_text/font_10x6_sans_serif.h \
    src/filter/filters/render_text/font_10x6_serif.h \
    src/filter/filters/render_text/font_5x3.h \
    src/filter/filters/render_text/font_fraps.h \
    src/filter/filters/source_fps_estimate/filter_source_fps_estimate.h \
    src/filter/filters/input_gate/filter_input_gate.h \
    src/filter/filters/input_gate/gui/filtergui_input_gate.h \
    src/filter/filters/kernel_3x3/filter_kernel_3x3.h \
    src/filter/filters/median/filter_median.h \
    src/filter/filters/output_gate/filter_output_gate.h \
    src/filter/filters/output_gate/gui/filtergui_output_gate.h \
    src/filter/filters/rotate/filter_rotate.h \
    src/filter/filters/sharpen/filter_sharpen.h \
    src/filter/filters/render_text/filter_render_text.h \
    src/filter/filters/render_text/gui/filtergui_render_text.h \
    src/filter/filters/unknown/filter_unknown.h \
    src/filter/filters/unsharp_mask/filter_unsharp_mask.h \
    src/main.h \
    src/scaler/scaler.h \
    src/capture/capture.h \
    src/display/display.h \
    src/common/log/log.h \
    src/common/abstract_gui.h \
    src/filter/filter.h \
    src/common/command_line/command_line.h \
    src/display/qt/persistent_settings.h \
    src/common/disk/csv.h \
    src/common/disk/disk.h \
    src/display/display.h \
    src/common/disk/file_writers/file_writer_filter_graph.h \
    src/common/disk/file_readers/file_reader_filter_graph.h \
    src/common/disk/file_reader.h \
    src/common/disk/file_streamer.h \
    src/common/disk/file_writer.h \
    src/common/refresh_rate.h \
    src/capture/video_presets.h \
    src/common/disk/file_writers/file_writer_video_presets.h \
    src/common/disk/file_readers/file_reader_video_presets.h \
    src/common/vcs_event/vcs_event.h \
    src/common/timer/timer.h

FORMS += \
    src/display/qt/widgets/ResolutionQuery.ui \
    src/display/qt/windows/ControlPanel.ui \
    src/display/qt/windows/ControlPanel/Capture/AliasResolutions.ui \
    src/display/qt/windows/ControlPanel/Capture/CaptureRate.ui \
    src/display/qt/windows/ControlPanel/Capture/InputChannel.ui \
    src/display/qt/windows/ControlPanel/Capture/InputResolution.ui \
    src/display/qt/windows/ControlPanel/Capture/SignalStatus.ui \
    src/display/qt/windows/ControlPanel/Output/Histogram.ui \
    src/display/qt/windows/ControlPanel/Output/Overlay.ui \
    src/display/qt/windows/ControlPanel/Output/Scaler.ui \
    src/display/qt/windows/ControlPanel/Output/Size.ui \
    src/display/qt/windows/ControlPanel/Output/Status.ui \
    src/display/qt/windows/ControlPanel/Output/Window.ui \
    src/display/qt/windows/OutputWindow.ui \
    src/display/qt/windows/ControlPanel/AboutVCS.ui \
    src/display/qt/windows/ControlPanel/Capture.ui \
    src/display/qt/windows/ControlPanel/FilterGraph.ui \
    src/display/qt/windows/ControlPanel/Output.ui \
    src/display/qt/windows/ControlPanel/VideoPresets.ui

QT += core gui widgets
TARGET = vcs
TEMPLATE = app
RESOURCES += src/display/qt/res.qrc
CONFIG += console release object_parallel_to_source
OBJECTS_DIR = generated_files
RCC_DIR = generated_files
MOC_DIR = generated_files
UI_DIR = generated_files

contains(DEFINES, CAPTURE_BACKEND_VISION_V4L) {
    INCLUDEPATH += $${DATAPATH_VISION_HEADER_PATH}
}

contains(DEFINES, VCS_USES_OPENCV) {
    INCLUDEPATH += $${OPENCV_HEADER_PATH}

    contains(DEFINES, VCS_DEBUG_BUILD) {
        # VCS's debug mode uses _GLIBCXX_DEBUG, which may be incompatible with
        # OpenCV (https://github.com/opencv/opencv/pull/9161) and in any case
        # triggers OpenCV's debug build guard. You can either ignore the build
        # guard, or take out _GLIBCXX_DEBUG (and _GLIBCXX_DEBUG_PEDANTIC).
        DEFINES += CV_IGNORE_DEBUG_BUILD_GUARD

        LIBS += -L$${OPENCV_DEBUG_LIB_PATH}
    } else {
        LIBS += -L$${OPENCV_LIB_PATH}
    }

    LIBS += \
        -lopencv_imgproc \
        -lopencv_highgui \
        -lopencv_core \
        -lopencv_photo \

    contains(DEFINES, CAPTURE_BACKEND_GPHOTO2) {
        LIBS += -lopencv_imgcodecs
    }

    contains(DEFINES, CAPTURE_BACKEND_VIRTUAL) {
        LIBS += -lopencv_imgcodecs
    }

    contains(DEFINES, CAPTURE_BACKEND_GENERIC_V4L) {
        LIBS += -lopencv_videoio
    }
}

contains(DEFINES, CAPTURE_BACKEND_GENERIC_V4L) {
    SOURCES += src/capture/generic_v4l/capture_generic_v4l.cpp
}

contains(DEFINES, CAPTURE_BACKEND_GPHOTO2) {
    SOURCES += src/capture/gphoto2/capture_gphoto2.cpp
    LIBS += -lgphoto2
}

contains(DEFINES, CAPTURE_BACKEND_VIRTUAL) {
    SOURCES += src/capture/virtual/capture_virtual.cpp
}

contains(DEFINES, CAPTURE_BACKEND_MMAP) {
    SOURCES += src/capture/mmap/capture_mmap.cpp
    LIBS += -lrt
}

contains(DEFINES, CAPTURE_BACKEND_VISION_V4L) {
    SOURCES += \
        src/capture/vision_v4l/capture_vision_v4l.cpp \
        src/capture/vision_v4l/input_channel_v4l.cpp \
        src/capture/vision_v4l/ic_v4l_video_parameters.cpp

    HEADERS += \
        src/capture/vision_v4l/input_channel_v4l.h \
        src/capture/vision_v4l/ic_v4l_video_parameters.h
}

contains(DEFINES, VCS_DEBUG_BUILD) {
    CONFIG = $$replace(CONFIG, "release", "debug")
    DEFINES += _GLIBCXX_DEBUG _GLIBCXX_DEBUG_PEDANTIC

    contains(DEFINES, VCS_DEBUG_BUILD_USES_SANITIZERS) {
        CONFIG += sanitizer sanitize_address sanitize_undefined
    }
} else {
    CONFIG += optimize_full
    QMAKE_CXXFLAGS += -march=native
}
