/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS scaler
 *
 * Scales captured frames to match a desired output resolution (for e.g. displaying on screen).
 *
 */

#include <algorithm>
#include <cstring>
#include <vector>
#include <cmath>
#include <opencv2/imgproc/imgproc.hpp>
#include "anti_tear/anti_tear.h"
#include "common/propagate/vcs_event.h"
#include "capture/capture.h"
#include "display/display.h"
#include "common/globals.h"
#include "filter/filter.h"
#include "filter/abstract_filter.h"
#include "filter/filters/output_scaler/filter_output_scaler.h"
#include "scaler/scaler.h"
#include "common/timer/timer.h"

vcs_event_c<const resolution_s&> ks_ev_new_output_resolution;
vcs_event_c<const image_s&> ks_ev_new_scaled_image;
vcs_event_c<unsigned> ks_ev_frames_per_second;
vcs_event_c<void> ks_ev_custom_scaler_enabled;
vcs_event_c<void> ks_ev_custom_scaler_disabled;

// For keeping track of the number of frames scaled per second.
static unsigned NUM_FRAMES_SCALED_PER_SECOND = 0;

// The image scalers available to VCS.
static const std::vector<image_scaler_s> KNOWN_SCALERS = {
    {"Nearest", filter_output_scaler_c::nearest},
    {"Linear",  filter_output_scaler_c::linear},
    {"Area",    filter_output_scaler_c::area},
    {"Cubic",   filter_output_scaler_c::cubic},
    {"Lanczos", filter_output_scaler_c::lanczos}
};

static const image_scaler_s *DEFAULT_SCALER = nullptr;

// The frame buffer where scaled frames are to be placed.
static uint8_t *FRAME_BUFFER_PIXELS = nullptr;
static resolution_s FRAME_BUFFER_RESOLUTION = {0};

 // The frame buffer's target bit depth.
static const u32 OUTPUT_BIT_DEPTH = 32;

// Whether the current output scaling is done via an output scaling filter that
// the user has specified in a filter chain.
static bool IS_CUSTOM_SCALER_ACTIVE = false;
static resolution_s CUSTOM_SCALER_FILTER_RESOLUTION = {0, 0};

// By default, the base resolution for scaling is the resolution of the input
// frame. But the user can also provide an override resolution that takes the
// place of the base resolution.
static resolution_s RESOLUTION_OVERRIDE = {640, 480};
static bool IS_RESOLUTION_OVERRIDE_ENABLED = false;

// An additional multiplier to be applied to the base resolution.
static double SCALING_MULTIPLIER = 1;
static bool IS_SCALING_MULTIPLIER_ENABLED = false;

resolution_s ks_base_resolution(void)
{
    return RESOLUTION_OVERRIDE;
}

// Returns the resolution at which the scaler will output after performing all the actions
// (e.g. relative scaling or aspect ratio correction) that it has been asked to.
//
resolution_s ks_output_resolution(void)
{
    if (IS_CUSTOM_SCALER_ACTIVE)
    {
        return CUSTOM_SCALER_FILTER_RESOLUTION;
    }

    const auto inRes = resolution_s::from_capture_device();
    resolution_s outRes = inRes;

    // Base resolution.
    if (IS_RESOLUTION_OVERRIDE_ENABLED)
    {
        outRes = RESOLUTION_OVERRIDE;
    }

    // Scaling.
    if (IS_SCALING_MULTIPLIER_ENABLED)
    {
        outRes.w = round(outRes.w * SCALING_MULTIPLIER);
        outRes.h = round(outRes.h * SCALING_MULTIPLIER);
    }

    // Bounds-check.
    {
        if (outRes.w > MAX_OUTPUT_WIDTH)
        {
            outRes.w = MAX_OUTPUT_HEIGHT;
        }
        else if (outRes.w < MIN_OUTPUT_WIDTH)
        {
            outRes.w = MIN_OUTPUT_WIDTH;
        }

        if (outRes.h > MAX_OUTPUT_HEIGHT)
        {
            outRes.h = MAX_OUTPUT_HEIGHT;
        }
        else if (outRes.h < MIN_OUTPUT_HEIGHT)
        {
            outRes.h = MIN_OUTPUT_HEIGHT;
        }
    }

    outRes.bpp = OUTPUT_BIT_DEPTH;

    return outRes;
}

// Replaces OpenCV's default error handler.
//
static int cv_error_handler(int status, const char* func_name, const char* err_msg, const char* file_name, int line, void* userdata)
{
    NBENE(("OpenCV reports an error: '%s'.", err_msg));
    k_assert(0, "OpenCV reported an error.");

    (void)func_name;
    (void)file_name;
    (void)userdata;
    (void)err_msg;
    (void)status;
    (void)line;

    return 1;
}

bool ks_is_custom_scaler_active(void)
{
    return IS_CUSTOM_SCALER_ACTIVE;
}

subsystem_releaser_t ks_initialize_scaler(void)
{
    DEBUG(("Initializing the scaler subsystem."));
    k_assert(!FRAME_BUFFER_PIXELS, "Attempting to doubly initialize the scaler subsystem.");

    cv::redirectError(cv_error_handler);

    FRAME_BUFFER_PIXELS = new uint8_t[MAX_NUM_BYTES_IN_OUTPUT_FRAME]();
    FRAME_BUFFER_RESOLUTION = {0, 0, OUTPUT_BIT_DEPTH};

    ks_set_default_scaler(KNOWN_SCALERS.at(0).name);

    kc_ev_new_captured_frame.listen([](const captured_frame_s &frame)
    {
        ks_scale_frame(frame);
    });

    kc_ev_invalid_signal.listen([]
    {
        ks_indicate_invalid_signal();
        kd_ev_dirty.fire();
    });

    kc_ev_signal_lost.listen([]
    {
        ks_indicate_no_signal();
        kd_ev_dirty.fire();
    });

    ks_ev_new_scaled_image.listen([]
    {
        NUM_FRAMES_SCALED_PER_SECOND++;
    });

    kt_timer(1000, [](const unsigned)
    {
        ks_ev_frames_per_second.fire(NUM_FRAMES_SCALED_PER_SECOND);

        NUM_FRAMES_SCALED_PER_SECOND = 0;
    });

    return []{};
}

// Takes the given image and scales it according to the scaler's current internal
// resolution settings. The scaled image is placed in the scaler's internal buffer.
//
void ks_scale_frame(const captured_frame_s &frame)
{
    resolution_s outputRes = ks_output_resolution();

    // Verify that we have a workable frame.
    {
        const auto minres = resolution_s::from_capture_device(": minimum");
        const auto maxres = resolution_s::from_capture_device(": maximum");

        if ((frame.r.bpp != 16) &&
            (frame.r.bpp != 24) &&
            (frame.r.bpp != 32))
        {
            NBENE(("Was asked to scale a frame with an incompatible bit depth (%u). Ignoring it.",
                    frame.r.bpp));
            return;
        }
        else if (outputRes.w > MAX_OUTPUT_WIDTH ||
                 outputRes.h > MAX_OUTPUT_HEIGHT)
        {
            NBENE(("Was asked to scale a frame with an output size (%u x %u) larger than the maximum allowed (%u x %u). Ignoring it.",
                    outputRes.w, outputRes.h, MAX_OUTPUT_WIDTH, MAX_OUTPUT_HEIGHT));
            return;
        }
        else if (!frame.pixels)
        {
            NBENE(("Was asked to scale a null frame. Ignoring it."));
            return;
        }
        else if (frame.r.bpp > MAX_OUTPUT_BPP)
        {
            NBENE(("Was asked to scale a frame with a color depth (%u bits) higher than that allowed (%u bits). Ignoring it.",
                   frame.r.bpp, MAX_OUTPUT_BPP));
            return;
        }
        else if (frame.r.w < minres.w ||
                 frame.r.h < minres.h)
        {
            NBENE(("Was asked to scale a frame with an input size (%u x %u) smaller than the minimum allowed (%u x %u). Ignoring it.",
                   frame.r.w, frame.r.h, minres.w, minres.h));
            return;
        }
        else if (frame.r.w > maxres.w ||
                 frame.r.h > maxres.h)
        {
            NBENE(("Was asked to scale a frame with an input size (%u x %u) larger than the maximum allowed (%u x %u). Ignoring it.",
                   frame.r.w, frame.r.h, maxres.w, maxres.h));
            return;
        }
        else if (!FRAME_BUFFER_PIXELS)
        {
            return;
        }
    }

    image_s imageToBeScaled = kat_anti_tear({frame.pixels, frame.r});
    abstract_filter_c *customScaler = kf_apply_matching_filter_chain(&imageToBeScaled);

    // If the active filter chain provided a custom output scaler, it'll override our
    // default scaler.
    if (customScaler)
    {
        k_assert(
            (customScaler->category() == filter_category_e::output_scaler),
            "Invalid filter category for custom output scaler."
        );

        if (!IS_CUSTOM_SCALER_ACTIVE)
        {
            ks_ev_custom_scaler_enabled.fire();
            IS_CUSTOM_SCALER_ACTIVE = true;
        }

        customScaler->apply(&imageToBeScaled);
        outputRes = CUSTOM_SCALER_FILTER_RESOLUTION = dynamic_cast<filter_output_scaler_c*>(customScaler)->output_resolution();
    }
    else
    {
        if (IS_CUSTOM_SCALER_ACTIVE)
        {
            ks_ev_custom_scaler_disabled.fire();
            IS_CUSTOM_SCALER_ACTIVE = false;
        }

        if (imageToBeScaled.resolution == outputRes)
        {
            std::memcpy(FRAME_BUFFER_PIXELS, imageToBeScaled.pixels, imageToBeScaled.byte_size());
        }
        else
        {
            k_assert(DEFAULT_SCALER, "A default scaler has not been defined.");

            image_s dstImage = image_s(FRAME_BUFFER_PIXELS, outputRes);
            DEFAULT_SCALER->apply(imageToBeScaled, &dstImage, {0, 0, 0, 0});
        }
    }

    if (FRAME_BUFFER_RESOLUTION != outputRes)
    {
        ks_ev_new_output_resolution.fire(outputRes);
        FRAME_BUFFER_RESOLUTION = outputRes;
    }

    ks_ev_new_scaled_image.fire(ks_scaler_frame_buffer());

    return;
}

void ks_set_base_resolution_enabled(const bool enabled)
{
    IS_RESOLUTION_OVERRIDE_ENABLED = enabled;
    kd_update_output_window_size();

    return;
}

void ks_set_base_resolution(const resolution_s &r)
{
    RESOLUTION_OVERRIDE = r;
    kd_update_output_window_size();

    return;
}

void ks_set_scaling_multiplier(const double s)
{
    SCALING_MULTIPLIER = s;
    kd_update_output_window_size();

    return;
}

double ks_scaling_multiplier(void)
{
    return SCALING_MULTIPLIER;
}

void ks_set_scaling_multiplier_enabled(const bool enabled)
{
    IS_SCALING_MULTIPLIER_ENABLED = enabled;
    kd_update_output_window_size();

    return;
}

static void clear_frame_buffer(void)
{
    k_assert(FRAME_BUFFER_PIXELS, "Couldn't access the output buffer: it was unexpectedly null.");

    memset(FRAME_BUFFER_PIXELS, 0, MAX_NUM_BYTES_IN_OUTPUT_FRAME);

    return;
}

void ks_indicate_no_signal(void)
{
    clear_frame_buffer();

    return;
}

void ks_indicate_invalid_signal(void)
{
    clear_frame_buffer();

    return;
}

image_s ks_scaler_frame_buffer(void)
{
    return {
        FRAME_BUFFER_PIXELS,
        FRAME_BUFFER_RESOLUTION
    };
}

// Returns a list of GUI-displayable names of the scalers that're available.
//
std::vector<std::string> ks_scaler_names(void)
{
    std::vector<std::string> names;

    for (uint i = 0; i < KNOWN_SCALERS.size(); i++)
    {
        names.push_back(KNOWN_SCALERS[i].name);
    }

    return names;
}

static const image_scaler_s* scaler_for_name_string(const std::string &name)
{
    const image_scaler_s *f = nullptr;

    k_assert(!KNOWN_SCALERS.empty(),
             "Could find no scaling filters to search.");

    for (size_t i = 0; i < KNOWN_SCALERS.size(); i++)
    {
        if (KNOWN_SCALERS[i].name == name)
        {
            f = &KNOWN_SCALERS[i];
            goto done;
        }
    }

    f = &KNOWN_SCALERS.at(0);
    NBENE(("Was unable to find a scaler called '%s'. "
           "Defaulting to the first scaler on the list (%s).",
           name.c_str(), f->name.c_str()));

    done:
    return f;
}

void ks_set_default_scaler(const std::string &name)
{
    DEFAULT_SCALER = scaler_for_name_string(name);

    return;
}

const image_scaler_s* ks_default_scaler(void)
{
    return DEFAULT_SCALER;
}
