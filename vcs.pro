# Whether this build of VCS is intended for release. Release builds may perform better
# but have fewer run-time error checks and contain less debug information.
#DEFINES += VCS_RELEASE_BUILD

linux {
    # Select one, depending on whether you're aiming to have VCS run on X11 or Wayland.
    # Some of VCS's functionality may be available on one but not the other.
    DEFINES += \
        VCS_FOR_X11
        #VCS_FOR_WAYLAND

    DEFINES += CAPTURE_BACKEND_VISION_V4L

    INCLUDEPATH += \
        # The base path for Datapath's Linux Vision driver header files. These are
        # bundled with the driver downloadable from Datapath's website. The header
        # files are expected to be in a "visionrgb/" subdirectory of this path, eg.
        # ".../visionrgb/include/rgb133control.h".
        $$(HOME)/sdk/

    LIBS += \
        -L/usr/local/lib/ \
        -lopencv_imgproc \
        -lopencv_highgui \
        -lopencv_core \
        -lopencv_photo

    contains(DEFINES, VCS_FOR_X11) {
        SOURCES += src/display/prevent_screensaver_x11.cpp
        LIBS += -lX11 -lXext
    } else {
        SOURCES += src/display/prevent_screensaver_null.cpp
    }
}

INCLUDEPATH += \
    $$PWD/src/ \
    $$PWD/src/display/qt/subclasses/

RESOURCES += \
    src/display/qt/res.qrc

SOURCES += \
    src/common/refresh_rate.cpp \
    src/common/vcs_event/vcs_event.cpp \
    src/display/qt/dialogs/components/capture_dialog/input_channel.cpp \
    src/display/qt/dialogs/components/capture_dialog/input_resolution.cpp \
    src/display/qt/dialogs/components/capture_dialog/signal_status.cpp \
    src/display/qt/dialogs/components/filter_graph_dialog/base_filter_graph_node.cpp \
    src/display/qt/dialogs/components/filter_graph_dialog/output_scaler_node.cpp \
    src/display/qt/dialogs/video_presets_dialog.cpp \
    src/display/qt/dialogs/components/window_options_dialog/window_renderer.cpp \
    src/display/qt/dialogs/components/window_options_dialog/window_scaler.cpp \
    src/display/qt/dialogs/components/window_options_dialog/window_size.cpp \
    src/display/qt/dialogs/window_options_dialog.cpp \
    src/display/qt/keyboard_shortcuts.cpp \
    src/display/qt/dialogs/capture_dialog.cpp \
    src/display/qt/subclasses/QFrame_qt_abstract_gui.cpp \
    src/display/qt/windows/control_panel_window.cpp \
    src/filter/filters/output_scaler/filter_output_scaler.cpp \
    src/filter/filters/output_scaler/gui/filtergui_output_scaler.cpp \
    src/filter/filters/render_text/filter_render_text.cpp \
    src/filter/filters/render_text/gui/filtergui_render_text.cpp \
    src/filter/filters/source_fps_estimate/filter_source_fps_estimate.cpp \
    src/filter/filters/source_fps_estimate/gui/filtergui_source_fps_estimate.cpp \
    src/main.cpp \
    src/display/display.cpp \
    src/display/qt/subclasses/QLabel_magnifying_glass.cpp \
    src/display/qt/subclasses/QComboBox_video_preset_list.cpp \
    src/display/qt/subclasses/QDialog_vcs_base_dialog.cpp \
    src/display/qt/subclasses/QGroupBox_parameter_grid.cpp \
    src/display/qt/subclasses/QMenu_dialog_file_menu.cpp \
    src/display/qt/windows/output_window.cpp \
    src/display/qt/dialogs/resolution_dialog.cpp \
    src/display/qt/d_main.cpp \
    src/display/qt/dialogs/overlay_dialog.cpp \
    src/anti_tear/anti_tear_multiple_per_frame.cpp \
    src/anti_tear/anti_tear_one_per_frame.cpp \
    src/anti_tear/anti_tearer.cpp \
    src/filter/abstract_filter.cpp \
    src/filter/filters/anti_tear/filter_anti_tear.cpp \
    src/filter/filters/anti_tear/gui/filtergui_anti_tear.cpp \
    src/filter/filters/blur/filter_blur.cpp \
    src/filter/filters/blur/gui/filtergui_blur.cpp \
    src/filter/filters/color_depth/filter_color_depth.cpp \
    src/filter/filters/color_depth/gui/filtergui_color_depth.cpp \
    src/filter/filters/crop/filter_crop.cpp \
    src/filter/filters/crop/gui/filtergui_crop.cpp \
    src/filter/filters/decimate/filter_decimate.cpp \
    src/filter/filters/decimate/gui/filtergui_decimate.cpp \
    src/filter/filters/delta_histogram/filter_delta_histogram.cpp \
    src/filter/filters/delta_histogram/gui/filtergui_delta_histogram.cpp \
    src/filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.cpp \
    src/filter/filters/denoise_nonlocal_means/gui/filtergui_denoise_nonlocal_means.cpp \
    src/filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.cpp \
    src/filter/filters/denoise_pixel_gate/gui/filtergui_denoise_pixel_gate.cpp \
    src/filter/filters/flip/filter_flip.cpp \
    src/filter/filters/flip/gui/filtergui_flip.cpp \
    src/filter/filters/input_gate/filter_input_gate.cpp \
    src/filter/filters/input_gate/gui/filtergui_input_gate.cpp \
    src/filter/filters/kernel_3x3/filter_kernel_3x3.cpp \
    src/filter/filters/kernel_3x3/gui/filtergui_kernel_3x3.cpp \
    src/filter/filters/median/filter_median.cpp \
    src/filter/filters/median/gui/filtergui_median.cpp \
    src/filter/filters/output_gate/filter_output_gate.cpp \
    src/filter/filters/output_gate/gui/filtergui_output_gate.cpp \
    src/filter/filters/rotate/filter_rotate.cpp \
    src/filter/filters/rotate/gui/filtergui_rotate.cpp \
    src/filter/filters/sharpen/filter_sharpen.cpp \
    src/filter/filters/sharpen/gui/filtergui_sharpen.cpp \
    src/filter/filters/unsharp_mask/filter_unsharp_mask.cpp \
    src/filter/filters/unsharp_mask/gui/filtergui_unsharp_mask.cpp \
    src/scaler/scaler.cpp \
    src/common/log/log.cpp \
    src/filter/filter.cpp \
    src/common/command_line/command_line.cpp \
    src/capture/capture.cpp \
    src/anti_tear/anti_tear.cpp \
    src/display/qt/persistent_settings.cpp \
    src/common/disk/disk.cpp \
    src/display/qt/subclasses/QOpenGLWidget_opengl_renderer.cpp \
    src/display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.cpp \
    src/display/qt/subclasses/QGraphicsScene_interactible_node_graph.cpp \
    src/display/qt/subclasses/QGraphicsView_interactible_node_graph_view.cpp \
    src/display/qt/dialogs/filter_graph_dialog.cpp \
    src/display/qt/dialogs/about_dialog.cpp \
    src/display/qt/subclasses/QTableWidget_property_table.cpp \
    src/common/disk/file_writers/file_writer_filter_graph_version_b.cpp \
    src/common/disk/file_reader.cpp \
    src/common/disk/file_readers/file_reader_filter_graph_version_b.cpp \
    src/common/disk/file_readers/file_reader_filter_graph_version_a.cpp \
    src/common/disk/file_writer.cpp \
    src/common/disk/file_writers/file_writer_video_params_version_a.cpp \
    src/common/disk/file_readers/file_reader_video_params_version_a.cpp \
    src/display/qt/dialogs/components/filter_graph_dialog/output_gate_node.cpp \
    src/display/qt/dialogs/components/filter_graph_dialog/input_gate_node.cpp \
    src/display/qt/dialogs/components/filter_graph_dialog/filter_node.cpp \
    src/capture/video_presets.cpp \
    src/common/disk/file_writers/file_writer_video_presets_version_a.cpp \
    src/common/disk/file_readers/file_reader_video_presets_version_a.cpp \
    src/common/timer/timer.cpp

HEADERS += \
    src/common/assert.h \
    src/common/globals.h \
    src/common/types.h \
    src/display/qt/dialogs/components/capture_dialog/input_channel.h \
    src/display/qt/dialogs/components/capture_dialog/input_resolution.h \
    src/display/qt/dialogs/components/capture_dialog/signal_status.h \
    src/display/qt/dialogs/components/filter_graph_dialog/base_filter_graph_node.h \
    src/display/qt/dialogs/components/filter_graph_dialog/output_scaler_node.h \
    src/display/qt/dialogs/video_presets_dialog.h \
    src/display/qt/dialogs/components/window_options_dialog/window_renderer.h \
    src/display/qt/dialogs/components/window_options_dialog/window_scaler.h \
    src/display/qt/dialogs/components/window_options_dialog/window_size.h \
    src/display/qt/dialogs/window_options_dialog.h \
    src/display/qt/keyboard_shortcuts.h \
    src/display/qt/dialogs/capture_dialog.h \
    src/display/qt/subclasses/QComboBox_video_preset_list.h \
    src/display/qt/subclasses/QDialog_vcs_base_dialog.h \
    src/display/qt/subclasses/QFrame_qt_abstract_gui.h \
    src/display/qt/subclasses/QGroupBox_parameter_grid.h \
    src/display/qt/subclasses/QLabel_magnifying_glass.h \
    src/display/qt/subclasses/QMenu_dialog_file_menu.h \
    src/display/qt/windows/control_panel_window.h \
    src/display/qt/windows/output_window.h \
    src/display/qt/dialogs/resolution_dialog.h \
    src/anti_tear/anti_tear_multiple_per_frame.h \
    src/anti_tear/anti_tear_one_per_frame.h \
    src/anti_tear/anti_tearer.h \
    src/filter/filters/anti_tear/filter_anti_tear.h \
    src/filter/filters/anti_tear/gui/filtergui_anti_tear.h \
    src/filter/filters/blur/filter_blur.h \
    src/filter/filters/blur/gui/filtergui_blur.h \
    src/filter/filters/color_depth/filter_color_depth.h \
    src/filter/filters/color_depth/gui/filtergui_color_depth.h \
    src/filter/filters/crop/filter_crop.h \
    src/filter/filters/crop/gui/filtergui_crop.h \
    src/filter/filters/decimate/filter_decimate.h \
    src/filter/filters/decimate/gui/filtergui_decimate.h \
    src/filter/filters/delta_histogram/filter_delta_histogram.h \
    src/filter/filters/delta_histogram/gui/filtergui_delta_histogram.h \
    src/filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.h \
    src/filter/filters/denoise_nonlocal_means/gui/filtergui_denoise_nonlocal_means.h \
    src/filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.h \
    src/filter/filters/denoise_pixel_gate/gui/filtergui_denoise_pixel_gate.h \
    src/filter/filters/filters.h \
    src/filter/filters/flip/filter_flip.h \
    src/filter/filters/flip/gui/filtergui_flip.h \
    src/filter/filters/output_scaler/filter_output_scaler.h \
    src/filter/filters/output_scaler/gui/filtergui_output_scaler.h \
    src/filter/filters/render_text/font.h \
    src/filter/filters/render_text/font_10x6_sans_serif.h \
    src/filter/filters/render_text/font_10x6_serif.h \
    src/filter/filters/render_text/font_5x3.h \
    src/filter/filters/render_text/font_fraps.h \
    src/filter/filters/source_fps_estimate/filter_source_fps_estimate.h \
    src/filter/filters/source_fps_estimate/gui/filtergui_source_fps_estimate.h \
    src/filter/filters/input_gate/filter_input_gate.h \
    src/filter/filters/input_gate/gui/filtergui_input_gate.h \
    src/filter/filters/kernel_3x3/filter_kernel_3x3.h \
    src/filter/filters/kernel_3x3/gui/filtergui_kernel_3x3.h \
    src/filter/filters/median/filter_median.h \
    src/filter/filters/median/gui/filtergui_median.h \
    src/filter/filters/output_gate/filter_output_gate.h \
    src/filter/filters/output_gate/gui/filtergui_output_gate.h \
    src/filter/filters/rotate/filter_rotate.h \
    src/filter/filters/rotate/gui/filtergui_rotate.h \
    src/filter/filters/sharpen/filter_sharpen.h \
    src/filter/filters/sharpen/gui/filtergui_sharpen.h \
    src/filter/filters/render_text/filter_render_text.h \
    src/filter/filters/render_text/gui/filtergui_render_text.h \
    src/filter/filters/unsharp_mask/filter_unsharp_mask.h \
    src/filter/filters/unsharp_mask/gui/filtergui_unsharp_mask.h \
    src/main.h \
    src/scaler/scaler.h \
    src/capture/capture.h \
    src/display/display.h \
    src/common/log/log.h \
    src/common/abstract_gui.h \
    src/display/qt/dialogs/overlay_dialog.h \
    src/anti_tear/anti_tear.h \
    src/filter/filter.h \
    src/common/command_line/command_line.h \
    src/display/qt/persistent_settings.h \
    src/display/qt/utility.h \
    src/common/disk/csv.h \
    src/common/disk/disk.h \
    src/display/qt/subclasses/QOpenGLWidget_opengl_renderer.h \
    src/display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h \
    src/display/qt/subclasses/QGraphicsScene_interactible_node_graph.h \
    src/display/qt/subclasses/QGraphicsView_interactible_node_graph_view.h \
    src/display/qt/dialogs/filter_graph_dialog.h \
    src/display/qt/dialogs/about_dialog.h \
    src/display/display.h \
    src/display/qt/subclasses/QTableWidget_property_table.h \
    src/common/disk/file_writers/file_writer_filter_graph.h \
    src/common/disk/file_readers/file_reader_filter_graph.h \
    src/common/disk/file_reader.h \
    src/common/disk/file_streamer.h \
    src/common/disk/file_readers/file_reader_video_params.h \
    src/common/disk/file_writers/file_writer_video_params.h \
    src/common/disk/file_writer.h \
    src/display/qt/dialogs/components/filter_graph_dialog/output_gate_node.h \
    src/display/qt/dialogs/components/filter_graph_dialog/input_gate_node.h \
    src/display/qt/dialogs/components/filter_graph_dialog/filter_node.h \
    src/common/refresh_rate.h \
    src/capture/video_presets.h \
    src/common/disk/file_writers/file_writer_video_presets.h \
    src/common/disk/file_readers/file_reader_video_presets.h \
    src/common/vcs_event/vcs_event.h \
    src/common/timer/timer.h

FORMS += \
    src/display/qt/dialogs/components/capture_dialog/ui/input_channel.ui \
    src/display/qt/dialogs/components/capture_dialog/ui/input_resolution.ui \
    src/display/qt/dialogs/components/capture_dialog/ui/signal_status.ui \
    src/display/qt/dialogs/ui/video_presets_dialog.ui \
    src/display/qt/dialogs/ui/window_options_dialog.ui \
    src/display/qt/dialogs/components/window_options_dialog/ui/window_renderer.ui \
    src/display/qt/dialogs/components/window_options_dialog/ui/window_scaler.ui \
    src/display/qt/dialogs/components/window_options_dialog/ui/window_size.ui \
    src/display/qt/dialogs/ui/capture_dialog.ui \
    src/display/qt/windows/ui/control_panel_window.ui \
    src/display/qt/windows/ui/output_window.ui \
    src/display/qt/dialogs/ui/overlay_dialog.ui \
    src/display/qt/dialogs/ui/resolution_dialog.ui \
    src/display/qt/dialogs/ui/filter_graph_dialog.ui \
    src/display/qt/dialogs/ui/about_dialog.ui \

contains(DEFINES, CAPTURE_BACKEND_VIRTUAL) {
    SOURCES += src/capture/virtual/capture_virtual.cpp
}

contains(DEFINES, CAPTURE_BACKEND_DOSBOX_MMAP) {
    SOURCES += src/capture/dosbox_mmap/capture_dosbox_mmap.cpp
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

QT += core gui widgets
TARGET = vcs
TEMPLATE = app
CONFIG += console release
QMAKE_CXXFLAGS += -pedantic -std=c++2a -Wno-missing-field-initializers
OBJECTS_DIR = generated_files
RCC_DIR = generated_files
MOC_DIR = generated_files
UI_DIR = generated_files

!contains(DEFINES, VCS_RELEASE_BUILD) {
    CONFIG = $$replace(CONFIG, "release", "debug")
    CONFIG += sanitizer sanitize_address sanitize_undefined
    DEFINES += _GLIBCXX_DEBUG _GLIBCXX_DEBUG_PEDANTIC
}
