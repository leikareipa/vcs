/*
 * 2018-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

/*
 * The display subsystem interface.
 *
 * The VCS GUI is responsible for providing the user with real-time means to
 * enter information (e.g. configuring capture parameters), for directing such
 * input to VCS, and for displaying captured frames and other information about
 * VCS's run-time state to the user.
 *
 * The display interface decouples the GUI's implementation from the rest of
 * VCS, making it possible to replace the GUI with fairly minimal modification
 * to the rest of VCS.
 *
 */

#ifndef VCS_DISPLAY_DISPLAY_H
#define VCS_DISPLAY_DISPLAY_H

#include <string>
#include <vector>
#include <cassert>
#include <cstring>
#include "common/vcs_event/vcs_event.h"
#include "filter/abstract_filter.h"
#include "main.h"

struct abstract_gui_s;

struct resolution_s
{
    unsigned w;
    unsigned h;

    static resolution_s from_capture_device_properties(const std::string &nameSpace = "");

    static void to_capture_device_properties(const resolution_s &resolution);

    bool operator==(const resolution_s &other) const;
    bool operator!=(const resolution_s &other) const;
    bool operator>(const resolution_s &other) const;
    bool operator<(const resolution_s &other) const;
};

struct image_s
{
    uint8_t *const pixels;
    const resolution_s resolution;
    const unsigned bitsPerPixel = 32;

    image_s(uint8_t *const pixels, const resolution_s &resolution) : pixels(pixels), resolution(resolution) {}

    unsigned bytes_per_pixel(void) const
    {
        return (this->bitsPerPixel / 8);
    }

    std::size_t byte_size(void) const
    {
        return (this->resolution.w * this->resolution.h * (bitsPerPixel / 8));
    }

    bool is_valid(void) const
    {
        return (
            this->pixels &&
            this->resolution.w &&
            this->resolution.h
        );
    }

    void operator=(const image_s &other)
    {
        assert((this->resolution == other.resolution) && "Attempting to copy images with mismatching resolutions.");
        std::memcpy(this->pixels, other.pixels, this->byte_size());
    }
};

struct filter_graph_option_s
{
    std::string propertyName;
    int value;

    filter_graph_option_s(const std::string propertyName, const int value)
    {
        this->propertyName = propertyName;
        this->value = value;

        return;
    }
};

void kd_add_control_panel_widget(const std::string &tabName, const std::string &widgetTitle, abstract_gui_s *widget);

// Asks the GUI to create and open the output window. The output window is a
// surface on which VCS's output frames are to be displayed by the GUI.
//
// If the GUI makes use of child windows and/or dialogs, this may be a good
// time to create them, as well.
subsystem_releaser_t kd_acquire_output_window(void);

// Visit each node in the graph and while doing so, group together such chains of
// filters that run from an input gate through one or more filters into an output
// gate. The chains will then be submitted to the filter handler for use in applying
// the filters to captured frames.
void kd_recalculate_filter_graph_chains(void);

// Asks the GUI to load video presets from the given file.
//
// The GUI is expected to inform VCS's video presets subsystem of the new
// presets.
void kd_load_video_presets(const std::string &filename);

// Asks the GUI to load a filter graph from the given file.
//
// The GUI is expected to inform VCS's filter subsystem of the new graph.
void kd_load_filter_graph(const std::string &filename);

void kd_update_output_window_title(void);

// Asks the GUI to execute one spin of its event loop.
//
// Spinning the event loop would involve e.g. repainting the output window,
// processing any user input, etc.
//
// If the GUI wants to match the capture's refresh rate, it should repaint its
// output only when VCS calls this function.
void kd_spin_event_loop(void);

// Asks the GUI to display an informational message to the user.
//
// The message should be deliverable with a headless GUI, i.e. requiring
// minimal prior GUI initialization.
void kd_show_headless_info_message(const char *const title, const char *const msg);

// Asks the GUI to display an error message to the user.
//
// The message should be deliverable with a headless GUI, i.e. requiring
// minimal prior GUI initialization.
void kd_show_headless_error_message(const char *const title, const char *const msg);

// Asks the GUI to display an assertion error message to the user.
//
// The message should be deliverable with a headless GUI, i.e. requiring
// minimal prior GUI initialization.
void kd_show_headless_assert_error_message(const char *const msg, const char * const filename, const uint lineNum);

bool kd_is_fullscreen(void);
std::string kd_fullscreen_ratio(void);
resolution_s kd_max_fullscreen_size(void);

#endif
