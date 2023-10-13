/*
 * 2020-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 * Implements capturing under Linux via the Datapath's Vision driver and the
 * Vision4Linux 2 API.
 *
 */

#include <unordered_map>
#include <cmath>
#include <atomic>
#include <vector>
#include <iostream>
#include <thread>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <cstring>
#include <chrono>
#include <poll.h>
#include "capture/vision_v4l/input_channel_v4l.h"
#include "capture/video_presets.h"
#include "capture/capture.h"
#include "capture/vision_v4l/ic_v4l_video_parameters.h"
#include "common/vcs_event/vcs_event.h"
#include "main.h"

#define INCLUDE_VISION
#include <rgb133control.h>
#include <rgb133v4l2.h>

// The input channel (/dev/videoX device) we're currently capturing from.
static input_channel_v4l_c *INPUT_CHANNEL = nullptr;

// The latest frame we've received from the capture device.
static captured_frame_s FRAME_BUFFER;

// Cumulative count of frames that were sent to us by the capture device but which
// VCS was too busy to process. Note that this count doesn't account for the missed
// frames on the current input channel, only on previous ones. The total number of
// missed frames over the program's execution is thus this value + the current input
// channel's value.
static unsigned NUM_MISSED_FRAMES = 0;

// Set to true if we've been asked to force a particular capture resolution. Will
// be reset to false once the resolution has been forced.
static bool FORCE_CUSTOM_RESOLUTION = false;

static std::vector<const char*> SUPPORTED_VIDEO_PROPERTIES = {
    "Horizontal size",
    "Horizontal position",
    "Vertical position",
    "Black level",
    "Phase",
    "Brightness",
    "Red brightness",
    "Green brightness",
    "Blue brightness",
    "Contrast",
    "Red contrast",
    "Green contrast",
    "Blue contrast",
};

static std::unordered_map<std::string, intptr_t> DEVICE_PROPERTIES = {
    {"width: minimum", MIN_CAPTURE_WIDTH},
    {"height: minimum", MIN_CAPTURE_HEIGHT},

    {"width: maximum", MAX_CAPTURE_WIDTH},
    {"height: maximum", MAX_CAPTURE_HEIGHT},

    {"width", 640},
    {"height", 480},

    {"supported video preset properties", intptr_t(&SUPPORTED_VIDEO_PROPERTIES)},
    {"supports video presets", true},
    {"supports channel switching", true},
    {"supports resolution switching", true},
};

static bool set_capture_resolution(const resolution_s r)
{
    k_assert(INPUT_CHANNEL, "Attempting to set the capture resolution on a null input channel.");
    k_assert(FORCE_CUSTOM_RESOLUTION, "Attempting to force the capture resolution without it having been requested.");

    FORCE_CUSTOM_RESOLUTION = false;

    if (!kc_has_signal())
    {
        DEBUG(("Was asked to set the capture resolution while there is no signal. Ignoring this."));
        return false;
    }

    // Validate the resolution.
    {
        const auto minRes = resolution_s::from_capture_device_properties(": minimum");
        const auto maxRes = resolution_s::from_capture_device_properties(": maximum");

        if (
            (r.w < minRes.w) ||
            (r.w > maxRes.w) ||
            (r.h < minRes.h) ||
            (r.h > maxRes.h)
        ){
            NBENE(("Failed to set the capture resolution: out of range"));
            goto fail;
        }
    }

    // Set the resolution.
    {
        v4l2_control control = {0};

        control.id = RGB133_V4L2_CID_HOR_TIME;
        control.value = r.w;
        if (!INPUT_CHANNEL->device_ioctl(VIDIOC_S_CTRL, &control))
        {
            NBENE(("Failed to set the capture width: device error."));
            return false;
        }

        control.id = RGB133_V4L2_CID_VER_TIME;
        control.value = r.h;
        if (!INPUT_CHANNEL->device_ioctl(VIDIOC_S_CTRL, &control))
        {
            NBENE(("Failed to set the capture height: device error."));
            goto fail;
        }
    }

    return true;

    fail:
    return false;
}

static void set_input_channel(const unsigned channelIdx)
{
    if (INPUT_CHANNEL)
    {
        NUM_MISSED_FRAMES += INPUT_CHANNEL->captureStatus.numNewFrameEventsSkipped;
        delete INPUT_CHANNEL;
    }

    INPUT_CHANNEL = new input_channel_v4l_c(
        (std::string("/dev/video") + std::to_string(unsigned(channelIdx))),
        3,
        &FRAME_BUFFER
    );

    ev_new_input_channel.fire(channelIdx);

    return;
}

intptr_t kc_device_property(const std::string &key)
{
    return (DEVICE_PROPERTIES.contains(key)? DEVICE_PROPERTIES.at(key) : 0);
}

bool kc_set_device_property(const std::string &key, intptr_t value)
{
    if (key == "width")
    {
        if (
            (value > MAX_CAPTURE_WIDTH) ||
            (value < MIN_CAPTURE_WIDTH)
        ){
            return false;
        }

        if (value != INPUT_CHANNEL->captureStatus.resolution.w)
        {
            FORCE_CUSTOM_RESOLUTION = true;
        }
        else
        {
            FRAME_BUFFER.resolution.w = value;
        }
    }
    else if (key == "height")
    {
        if (
            (value > MAX_CAPTURE_HEIGHT) ||
            (value < MIN_CAPTURE_HEIGHT)
        ){
            return false;
        }

        if (value != INPUT_CHANNEL->captureStatus.resolution.h)
        {
            FORCE_CUSTOM_RESOLUTION = true;
        }
        else
        {
            FRAME_BUFFER.resolution.h = value;
        }
    }
    else if (key == "channel")
    {
        set_input_channel(value);
    }
    // Video parameters.
    else
    {
        static const auto set_video_param = [](const int value, const ic_v4l_controls_c::type_e parameterType)->bool
        {
            if (!kc_has_signal())
            {
                DEBUG(("Was asked to set capture video params while there was no signal. Ignoring the request."));
                return false;
            }

            return INPUT_CHANNEL->captureStatus.videoParameters.set_value(value, parameterType);
        };

        static const auto videoParams = std::vector<std::pair<std::string, ic_v4l_controls_c::type_e>>{
            {"Brightness",          ic_v4l_controls_c::type_e::brightness},
            {"Contrast",            ic_v4l_controls_c::type_e::contrast},
            {"Red brightness",      ic_v4l_controls_c::type_e::red_brightness},
            {"Green brightness",    ic_v4l_controls_c::type_e::green_brightness},
            {"Blue brightness",     ic_v4l_controls_c::type_e::blue_brightness},
            {"Red contrast",        ic_v4l_controls_c::type_e::red_contrast},
            {"Green contrast",      ic_v4l_controls_c::type_e::green_contrast},
            {"Blue contrast",       ic_v4l_controls_c::type_e::blue_contrast},
            {"Horizontal size",     ic_v4l_controls_c::type_e::horizontal_size},
            {"Horizontal position", ic_v4l_controls_c::type_e::horizontal_position},
            {"Vertical position",   ic_v4l_controls_c::type_e::vertical_position},
            {"Phase",               ic_v4l_controls_c::type_e::phase},
            {"Black level",         ic_v4l_controls_c::type_e::black_level},
        };

        for (const auto &videoParam: videoParams)
        {
            if (key == videoParam.first)
            {
                set_video_param(value, videoParam.second);
                break;
            }
        }
    }

    DEVICE_PROPERTIES[key] = value;

    return true;
}

capture_event_e kc_process_next_capture_event(void)
{
    if (!INPUT_CHANNEL)
    {
        ev_unrecoverable_capture_error.fire();
        return capture_event_e::unrecoverable_error;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::unrecoverable_error))
    {
        INPUT_CHANNEL->captureStatus.invalidDevice = true;
        ev_unrecoverable_capture_error.fire();
        return capture_event_e::unrecoverable_error;
    }
    else if (FORCE_CUSTOM_RESOLUTION)
    {
        set_capture_resolution(resolution_s{
            .w = unsigned(kc_device_property("width")),
            .h = unsigned(kc_device_property("height"))
        });
        return capture_event_e::none;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::new_video_mode))
    {
        // Re-create the input channel for the new video mode.
        kc_set_device_property("channel", kc_device_property("channel"));

        return capture_event_e::new_video_mode;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::signal_lost))
    {
        kc_set_device_property("has signal", false);
        ev_capture_signal_lost.fire();
        return capture_event_e::signal_lost;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::signal_gained))
    {
        kc_set_device_property("has signal", true);
        ev_capture_signal_gained.fire();
        return capture_event_e::signal_gained;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::invalid_signal))
    {
        kc_set_device_property("has signal", false);
        ev_invalid_capture_signal.fire();
        return capture_event_e::invalid_signal;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::invalid_device))
    {
        ev_invalid_capture_device.fire();
        return capture_event_e::invalid_device;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::new_frame))
    {
        ev_new_captured_frame.fire(FRAME_BUFFER);
        return capture_event_e::new_frame;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::sleep))
    {
        return capture_event_e::sleep;
    }

    return capture_event_e::none;
}

uint kc_dropped_frames_count(void)
{
    k_assert(INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    return (NUM_MISSED_FRAMES + INPUT_CHANNEL->captureStatus.numNewFrameEventsSkipped);
}

void kc_initialize_device(void)
{
    DEBUG(("Initializing the Vision/V4L capture device."));

    k_assert(!FRAME_BUFFER.pixels, "Attempting to doubly initialize the capture device.");
    FRAME_BUFFER.pixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    // Start capturing.
    kc_set_device_property("channel", INPUT_CHANNEL_IDX);
    k_assert(INPUT_CHANNEL, "Failed to initialize the hardware input channel.");

    // Initialize value ranges for video parameters.
    //
    // Note: Querying these values from V4L at runtime seems to work a little
    // unreliably, so for now let's just use hard-coded values. These values
    // have been selected with the VisionRGB-E1S in mind.
    {
        ic_v4l_controls_c *const videoParams = &INPUT_CHANNEL->captureStatus.videoParameters;

        videoParams->update();

        analog_properties_s::to_capture_device_properties({
            {"Brightness", 32},
            {"Contrast", 128},
            {"Red brightness", 128},
            {"Green brightness", 128},
            {"Blue brightness", 128},
            {"Red contrast", 256},
            {"Green contrast", 256},
            {"Blue contrast", 256},
            {"Horizontal size", 900},
            {"Horizontal position", 112},
            {"Vertical position", 36},
            {"Phase", 0},
            {"Black level", 8},
        }, ": default");

        analog_properties_s::to_capture_device_properties({
            {"Brightness", 0},
            {"Contrast", 0},
            {"Red brightness", 0},
            {"Green brightness", 0},
            {"Blue brightness", 0},
            {"Red contrast", 0},
            {"Green contrast", 0},
            {"Blue contrast", 0},
            {"Horizontal size", 100},
            {"Horizontal position", 1},
            {"Vertical position", 1},
            {"Phase", 0},
            {"Black level", 1},
        }, ": minimum");

        analog_properties_s::to_capture_device_properties({
            {"Brightness", 63},
            {"Contrast", 255},
            {"Red brightness", 255},
            {"Green brightness", 255},
            {"Blue brightness", 255},
            {"Red contrast", 511},
            {"Green contrast", 511},
            {"Blue contrast", 511},
            {"Horizontal size", 4095},
            {"Horizontal position", 1200},
            {"Vertical position", 63},
            {"Phase", 31},
            {"Black level", 255},
        }, ": maximum");
    }

    // Listen for relevant events.
    {
        ev_capture_signal_gained.listen([]
        {
            k_assert(INPUT_CHANNEL, "Attempting to set input channel parameters on a null channel.");

            INPUT_CHANNEL->captureStatus.videoParameters.update();

            ev_new_proposed_video_mode.fire({
                INPUT_CHANNEL->captureStatus.resolution,
                INPUT_CHANNEL->captureStatus.refreshRate,
            });
        });

        ev_frame_processing_finished.listen([]
        {
            k_assert(INPUT_CHANNEL, "Attempting to set input channel parameters on a null channel.");
            INPUT_CHANNEL->captureStatus.numFramesProcessed++;
        });

        ev_new_video_mode.listen([](const video_mode_s &mode)
        {
            resolution_s::to_capture_device_properties(mode.resolution);
            refresh_rate_s::to_capture_device_properties(mode.refreshRate);
        });
    }

    return;
}

bool kc_release_device(void)
{
    delete INPUT_CHANNEL;
    delete [] FRAME_BUFFER.pixels;

    return true;
}

const captured_frame_s& kc_frame_buffer(void)
{
    return FRAME_BUFFER;
}
