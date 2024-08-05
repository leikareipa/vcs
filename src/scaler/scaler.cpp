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
#include "capture/capture.h"
#include "display/display.h"
#include "common/globals.h"
#include "filter/filter.h"
#include "filter/abstract_filter.h"
#include "filter/filters/output_scaler/filter_output_scaler.h"
#include "filter/filters/render_text/font_10x6_serif.h"
#include "filter/filters/render_text/font_10x6_sans_serif.h"
#include "scaler/scaler.h"
#include "common/timer/timer.h"

// For keeping track of the number of frames scaled per second.
static unsigned NUM_FRAMES_SCALED_PER_SECOND = 0;

static std::string STATUS_MESSAGE = "";

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

    const auto inRes = resolution_s::from_capture_device_properties();
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
    FRAME_BUFFER_RESOLUTION = {.w = 0, .h = 0};

    ks_set_default_scaler(KNOWN_SCALERS.at(0).name);

    ev_new_captured_frame.listen([](const captured_frame_s &frame)
    {
        if (kc_has_signal())
        {
            ks_scale_frame(frame);
        }
        else
        {
            DEBUG(("Was asked to scale a frame while there was no signal. Ignoring this."));
            return;
        }
    });

    ev_invalid_capture_signal.listen([]
    {
        ks_indicate_status("INVALID SIGNAL");
        ev_dirty_output_window.fire();
    });

    ev_new_output_resolution.listen([]
    {
        if (!kc_has_signal())
        {
            ks_indicate_status(STATUS_MESSAGE);
            ev_dirty_output_window.fire();
        }
    });

    ev_capture_signal_lost.listen([]
    {
        ks_indicate_status("NO SIGNAL");
        ev_dirty_output_window.fire();
    });

    ev_new_output_image.listen([]
    {
        NUM_FRAMES_SCALED_PER_SECOND++;
    });

    kt_timer(1000, [](const unsigned timeElapsedMs)
    {
        ev_frames_per_second.fire(NUM_FRAMES_SCALED_PER_SECOND * (1000.0 / timeElapsedMs));
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
        const auto minres = resolution_s::from_capture_device_properties(": minimum");
        const auto maxres = resolution_s::from_capture_device_properties(": maximum");

        if (outputRes.w > MAX_OUTPUT_WIDTH ||
            outputRes.h > MAX_OUTPUT_HEIGHT)
        {
            DEBUG(("Was asked to scale a frame with an output size (%u x %u) larger than the maximum allowed (%u x %u). Ignoring it.",
                    outputRes.w, outputRes.h, MAX_OUTPUT_WIDTH, MAX_OUTPUT_HEIGHT));
            return;
        }
        else if (!frame.pixels)
        {
            DEBUG(("Was asked to scale a null frame. Ignoring it."));
            return;
        }
        else if (frame.resolution.w < minres.w ||
                 frame.resolution.h < minres.h)
        {
            DEBUG(("Was asked to scale a frame with an input size (%u x %u) smaller than the minimum allowed (%u x %u). Ignoring it.",
                   frame.resolution.w, frame.resolution.h, minres.w, minres.h));
            return;
        }
        else if (frame.resolution.w > maxres.w ||
                 frame.resolution.h > maxres.h)
        {
            DEBUG(("Was asked to scale a frame with an input size (%u x %u) larger than the maximum allowed (%u x %u). Ignoring it.",
                   frame.resolution.w, frame.resolution.h, maxres.w, maxres.h));
            return;
        }
        else if (!FRAME_BUFFER_PIXELS)
        {
            DEBUG(("Was asked to scale a frame before the scaler's frame buffer had been initialized. Ignoring it."));
            return;
        }
    }

    image_s imageToBeScaled(frame.pixels, frame.resolution);
    abstract_filter_c *customScaler = kf_apply_matching_filter_chain(&imageToBeScaled);

    // If there was a matching filter chain and it provides a custom output scaler,
    // it'll override our default scaler.
    if (customScaler)
    {
        k_assert(
            (customScaler->category() == filter_category_e::output_scaler),
            "Invalid filter category for custom output scaler."
        );

        if (!IS_CUSTOM_SCALER_ACTIVE)
        {
            ev_custom_output_scaler_enabled.fire();
            IS_CUSTOM_SCALER_ACTIVE = true;
        }

        customScaler->apply(&imageToBeScaled);
        outputRes = CUSTOM_SCALER_FILTER_RESOLUTION = dynamic_cast<filter_output_scaler_c*>(customScaler)->output_resolution();
    }
    else
    {
        if (IS_CUSTOM_SCALER_ACTIVE)
        {
            ev_custom_output_scaler_disabled.fire();
            IS_CUSTOM_SCALER_ACTIVE = false;
        }

        if (imageToBeScaled.resolution == outputRes)
        {
            std::memcpy(FRAME_BUFFER_PIXELS, imageToBeScaled.pixels, imageToBeScaled.byte_size());
        }
        else
        {
            k_assert(DEFAULT_SCALER, "A default scaler has not been defined.");

            if (kd_is_fullscreen())
            {
                const std::string ratio = kd_fullscreen_ratio();

                if (ratio == "Desktop size")
                {
                    image_s dstImage = image_s(FRAME_BUFFER_PIXELS, outputRes);
                    DEFAULT_SCALER->apply(imageToBeScaled, &dstImage, {0, 0, 0, 0});
                }
                else
                {
                    double aspectX = 4;
                    double aspectY = 3;

                    if (ratio == "4:3")
                    {
                        aspectX = 4;
                        aspectY = 3;
                    }
                    else if (ratio == "16:9")
                    {
                        aspectX = 16;
                        aspectY = 9;
                    }
                    else
                    {
                        const int gcd = std::gcd(frame.resolution.w, frame.resolution.h);
                        aspectX = (frame.resolution.w / gcd);
                        aspectY = (frame.resolution.h / gcd);
                    }

                    resolution_s ratiodRes = {
                        .w = unsigned(std::round(outputRes.h * (aspectX / aspectY))),
                        .h = outputRes.h
                    };

                    if (ratiodRes.w > outputRes.w)
                    {
                        ratiodRes = {
                            .w = outputRes.w,
                            .h = unsigned(std::round(outputRes.w * (aspectY / aspectX)))
                        };
                    }

                    const unsigned hPadding = ((outputRes.w - ratiodRes.w) / 2);
                    image_s dstImage = image_s(FRAME_BUFFER_PIXELS, ratiodRes);
                    DEFAULT_SCALER->apply(imageToBeScaled, &dstImage, {0, hPadding, 0, hPadding});
                }
            }
            else
            {
                image_s dstImage = image_s(FRAME_BUFFER_PIXELS, outputRes);
                DEFAULT_SCALER->apply(imageToBeScaled, &dstImage, {0, 0, 0, 0});
            }
        }
    }

    if (FRAME_BUFFER_RESOLUTION != outputRes)
    {
        ev_new_output_resolution.fire(outputRes);
        FRAME_BUFFER_RESOLUTION = outputRes;
    }

    ev_new_output_image.fire(ks_scaler_frame_buffer());

    return;
}

void ks_set_base_resolution_enabled(const bool enabled)
{
    IS_RESOLUTION_OVERRIDE_ENABLED = enabled;
    ev_new_output_resolution.fire(ks_output_resolution());

    return;
}

void ks_set_base_resolution(const resolution_s &r)
{
    RESOLUTION_OVERRIDE = r;
    ev_new_output_resolution.fire(ks_output_resolution());

    return;
}

void ks_set_scaling_multiplier(const double s)
{
    SCALING_MULTIPLIER = s;
    ev_new_output_resolution.fire(ks_output_resolution());

    return;
}

double ks_scaling_multiplier(void)
{
    return SCALING_MULTIPLIER;
}

void ks_set_scaling_multiplier_enabled(const bool enabled)
{
    IS_SCALING_MULTIPLIER_ENABLED = enabled;
    ev_new_output_resolution.fire(ks_output_resolution());

    return;
}

static void clear_frame_buffer(void)
{
    k_assert(FRAME_BUFFER_PIXELS, "Couldn't access the output buffer: it was unexpectedly null.");

    memset(FRAME_BUFFER_PIXELS, 0, MAX_NUM_BYTES_IN_OUTPUT_FRAME);

    return;
}

void ks_indicate_status(const std::string &message)
{
    STATUS_MESSAGE = message;

    const unsigned fontScale = 7;
    static font_c *const font = new font_5x3_c;

    clear_frame_buffer();

    FRAME_BUFFER_RESOLUTION = ks_output_resolution();
    image_s image = {
        .pixels = FRAME_BUFFER_PIXELS,
        .resolution = FRAME_BUFFER_RESOLUTION
    };
    font->render(
        message,
        &image,
        (image.resolution.w / 2),
        (image.resolution.h / 2) - ((font->height_of(message) * fontScale) / 2),
        fontScale,
        {0, 0, 0},
        {160, 160, 160},
        filter_render_text_c::ALIGN_CENTER
    );

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

    // Redraw the most recent frame using the new scaler.
    if (kc_has_signal())
    {
        ks_scale_frame(kc_frame_buffer());
    }

    return;
}

const image_scaler_s* ks_default_scaler(void)
{
    return DEFAULT_SCALER;
}
