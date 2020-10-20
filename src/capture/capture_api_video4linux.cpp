/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 * Implements capturing under Linux via the Datapath's Vision driver and the
 * Vision4Linux 2 API.
 *
 * Initialization happens in ::initialize(). The capture thread runs in
 * capture_function().
 *
 */

#ifdef CAPTURE_API_VIDEO4LINUX

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
#include "capture/capture_api_video4linux.h"

#define INCLUDE_VISION
#include <visionrgb/include/rgb133v4l2.h>

static std::atomic<unsigned int> NUM_FRAMES_PROCESSED(0);
static std::atomic<unsigned int> NUM_FRAMES_CAPTURED(0);

// The number of frames the capture hardware has sent which VCS was too busy to
// receive and so which we had to skip.
static std::atomic<unsigned int> NUM_NEW_FRAME_EVENTS_SKIPPED(0);

// A value returned from open("/dev/videoX") - capture channel for the
// selected input.
static int CAPTURE_HANDLE = 0;

static bool NO_SIGNAL = false;
static bool INVALID_SIGNAL = false;

// The current (or most recent) capture resolution. This is a cached variable
// intended to reduce calls to the Vision API - it gets updated whenever the
// API tells us of a new capture resolution without us specifically polling
// the API for it.
static resolution_s CAPTURE_RESOLUTION = {640, 480, 32};

// The current refresh rate, multiplied by 1000.
static refresh_rate_s REFRESH_RATE = refresh_rate_s(0);

static capture_pixel_format_e CAPTURE_PIXEL_FORMAT = capture_pixel_format_e::rgb_888;

// The latest frame we've received from the capture device.
static captured_frame_s FRAME_BUFFER;

// A back buffer area for the capture device to capture into.
static struct capture_back_buffer_s
{
    const unsigned numPages = 2;

    heap_bytes_s<u8>& page(const unsigned idx)
    {
        k_assert((idx < this->numPages), "Accessing back buffer pages out of bounds.");

        return this->pages[idx];
    }

    void allocate(void)
    {
        for (auto &page: this->pages)
        {
            page.alloc(MAX_FRAME_SIZE);
        }
    }

    void release(void)
    {
        for (auto &page: this->pages)
        {
            if (!page.is_null())
            {
                page.release_memory();
            }
        }
    }

    capture_back_buffer_s(void)
    {
        this->pages.resize(this->numPages);
    }

    ~capture_back_buffer_s(void)
    {
        this->release();
    }

private:
    std::vector<heap_bytes_s<u8>> pages;
} CAPTURE_BACK_BUFFER;

// Flags that the capture thread will set to convey to the VCS thread the
// various capture events it detects during capture.
static bool CAPTURE_EVENT_FLAG[static_cast<int>(capture_event_e::num_enumerators)] = {false};

static struct signal_parameters_s
{
    // These correspond to the signal parameters recognized by VCS.
    enum class parameter_type_e
    {
        horizontal_size,
        horizontal_position,
        vertical_position,
        phase,
        black_level,
        brightness,
        contrast,
        red_brightness,
        red_contrast,
        green_brightness,
        green_contrast,
        blue_brightness,
        blue_contrast,

        unknown,
    };

    // Ask the capture device to change the given parameters's value. Returns true
    // on success; false otherwise.
    bool set_value(const int newValue, const parameter_type_e parameterType)
    {
        // If we don't know the this parameter.
        if (this->v4l_id(parameterType) < 0)
        {
            return false;
        }

        v4l2_control v4lc = {};
        v4lc.id = this->v4l_id(parameterType);
        v4lc.value = newValue;

        return bool(ioctl(CAPTURE_HANDLE, VIDIOC_S_CTRL, &v4lc) == 0);
    }

    int value(const parameter_type_e parameterType)
    {
        try
        {
            return this->parameters.at(parameterType).currentValue;
        }
        catch(...)
        {
            return 0;
        }
    }

    int default_value(const parameter_type_e parameterType)
    {
        try
        {
            return this->parameters.at(parameterType).defaultValue;
        }
        catch(...)
        {
            return 0;
        }
    }

    int minimum_value(const parameter_type_e parameterType)
    {
        try
        {
            return this->parameters.at(parameterType).minimumValue;
        }
        catch(...)
        {
            return 0;
        }
    }

    int maximum_value(const parameter_type_e parameterType)
    {
        try
        {
            return this->parameters.at(parameterType).maximumValue;
        }
        catch(...)
        {
            return 0;
        }
    }

    int v4l_id(const parameter_type_e parameterType)
    {
        try
        {
            return this->parameters.at(parameterType).v4lId;
        }
        catch(...)
        {
            return -1;
        }
    }

    std::string name(const parameter_type_e parameterType)
    {
        try
        {
            return this->parameters.at(parameterType).name;
        }
        catch(...)
        {
            return "unknown";
        }
    }

    // Polls the capture device for the current signal parameters' values.
    void update(void)
    {
        this->parameters.clear();

        const auto enumerate_parameters = [this](const unsigned startID, const unsigned endID)
        {
            for (unsigned i = startID; i < endID; i++)
            {
                v4l2_queryctrl query = {};
                query.id = i;

                if (ioctl(CAPTURE_HANDLE, VIDIOC_QUERYCTRL, &query) == 0)
                {
                    if (!(query.flags & V4L2_CTRL_FLAG_DISABLED) &&
                        !(query.flags & V4L2_CTRL_FLAG_READ_ONLY) &&
                        !(query.flags & V4L2_CTRL_FLAG_INACTIVE) &&
                        !(query.flags & V4L2_CTRL_FLAG_HAS_PAYLOAD) &&
                        !(query.flags & V4L2_CTRL_FLAG_WRITE_ONLY))
                    {
                        signal_parameter_s parameter;

                        parameter.name = (char*)query.name;
                        parameter.v4lId = i;
                        parameter.minimumValue = query.minimum;
                        parameter.maximumValue = query.maximum;
                        parameter.defaultValue = query.default_value;
                        parameter.stepSize = query.step;

                        // Attempt to get the control's current value.
                        {
                            v4l2_control v4lc = {};
                            v4lc.id = i;

                            if (ioctl(CAPTURE_HANDLE, VIDIOC_G_CTRL, &v4lc) == 0)
                            {
                                parameter.currentValue = v4lc.value;
                            }
                            else
                            {
                                parameter.currentValue = 0;
                            }
                        }

                        // Standardize the control names.
                        for (auto &chr: parameter.name)
                        {
                            chr = ((chr == ' ')? '_' : (char)std::tolower(chr));
                        }

                        this->parameters[this->type_for_name(parameter.name)] = parameter;
                    }
                }
            }
        };

        enumerate_parameters(V4L2_CID_BASE, V4L2_CID_LASTP1);
        enumerate_parameters(V4L2_CID_PRIVATE_BASE, V4L2_CID_PRIVATE_LASTP1);

        return;
    }

private:
    parameter_type_e type_for_name(const std::string &name)
    {
        if (name == "horizontal_size")     return parameter_type_e::horizontal_size;
        if (name == "horizontal_position") return parameter_type_e::horizontal_position;
        if (name == "vertical_position")   return parameter_type_e::vertical_position;
        if (name == "phase")               return parameter_type_e::phase;
        if (name == "black_level")         return parameter_type_e::black_level;
        if (name == "brightness")          return parameter_type_e::brightness;
        if (name == "contrast")            return parameter_type_e::contrast;
        if (name == "red_brightness")      return parameter_type_e::red_brightness;
        if (name == "red_contrast")        return parameter_type_e::red_contrast;
        if (name == "green_brightness")    return parameter_type_e::green_brightness;
        if (name == "green_contrast")      return parameter_type_e::green_contrast;
        if (name == "blue_brightness")     return parameter_type_e::blue_brightness;
        if (name == "blue_contrast")       return parameter_type_e::blue_contrast;

        return parameter_type_e::unknown;
    }

    // Data mined from the v4l2_queryctrl struct.
    struct signal_parameter_s
    {
        std::string name;
        int currentValue;
        int minimumValue;
        int maximumValue;
        int defaultValue;
        int stepSize;
        /// TODO: Implement v4l2_queryctrl flags.

        int v4lId;
    };

    std::unordered_map<parameter_type_e, signal_parameter_s> parameters;
} SIGNAL_CONTROLS;

// Marks the given capture event as having occurred.
static void push_capture_event(capture_event_e event)
{
    CAPTURE_EVENT_FLAG[static_cast<int>(event)] = true;

    return;
}

// Removes the given capture event from the event 'queue', and returns its value.
static bool pop_capture_event(capture_event_e event)
{
    const bool eventOccurred = CAPTURE_EVENT_FLAG[static_cast<int>(event)];

    CAPTURE_EVENT_FLAG[static_cast<int>(event)] = false;

    return eventOccurred;
}

// Converts VCS's pixel format enumerator into Video4Linux's pixel format identifier.
static u32 pixel_format_to_v4l_pixel_format(capture_pixel_format_e fmt)
{
    switch (fmt)
    {
        case capture_pixel_format_e::rgb_555: return V4L2_PIX_FMT_RGB555;
        case capture_pixel_format_e::rgb_565: return V4L2_PIX_FMT_RGB565;
        case capture_pixel_format_e::rgb_888: return V4L2_PIX_FMT_RGB32;
        default: k_assert(0, "Unknown pixel format."); return V4L2_PIX_FMT_RGB32;
    }
}

static bool capture_apicall(const unsigned long request, void *data)
{
    const int retVal = ioctl(CAPTURE_HANDLE, request, data);

    if (retVal < 0)
    {
        NBENE(("An apicall (request %d) failed (error %d).", request, retVal));
        return false;
    }

    return true;
}

// Stops the capture stream.
static bool stream_off(void)
{
    v4l2_buf_type bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (!capture_apicall(VIDIOC_STREAMOFF, &bufType))
    {
        NBENE(("Couldn't stop the capture stream."));
        return false;
    }

    return true;
}

// Starts the capture stream.
static bool stream_on(void)
{
    v4l2_buf_type bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (!capture_apicall(VIDIOC_STREAMON, &bufType))
    {
        NBENE(("Couldn't start the capture stream."));
        return false;
    }

    return true;
}

// Returns true if the given v4l2 capabilities match a vision device
static bool is_vision_caps(v4l2_capability* card_caps) 
{
    return strcmp((const char*)card_caps->driver, "Vision") == 0 && // Verify that card is using "Vision" driver
        strcmp((const char*)card_caps->card, "Vision Control") != 0; // Verify that card is not "Vision Control" device
}

bool capture_api_video4linux_s::unqueue_capture_buffers(void)
{
    // Tell the capture device we want it to use capture buffers we've allocated.
    {
        v4l2_requestbuffers buf;

        memset(&buf, 0, sizeof(buf));
        buf.count = 0;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;

        if (!capture_apicall(VIDIOC_REQBUFS, &buf))
        {
            NBENE(("Failed to unqueue capture buffers (error %d).", errno));
            goto fail;
        }
    }

    return true;

    fail:
    return false;
}

bool capture_api_video4linux_s::enqueue_capture_buffers(void)
{
    // Set the capture resolution to the highest possible - it seems this is
    // needed to get the capture device to assign the maximum possible frame
    // size for the buffers.
    {
        const resolution_s maxCaptureResolution = this->get_maximum_resolution();

        v4l2_format format;
        memset(&format, 0, sizeof(format));
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (!capture_apicall(VIDIOC_G_FMT, &format))
        {
            NBENE(("Failed to query the current capture format for enqueuing capture buffers (error %d).", errno));
            goto fail;
        }

        format.fmt.pix.width = maxCaptureResolution.w;
        format.fmt.pix.height = maxCaptureResolution.h;
        format.fmt.pix.pixelformat = pixel_format_to_v4l_pixel_format(CAPTURE_PIXEL_FORMAT);
        format.fmt.pix.field = V4L2_FIELD_NONE;

        if (!capture_apicall(VIDIOC_S_FMT, &format) ||
            !capture_apicall(VIDIOC_G_FMT, &format))
        {
            NBENE(("Failed to set the current capture format for enqueuing capture buffers(error %d).", errno));
            goto fail;
        }

        if ((format.fmt.pix.width != maxCaptureResolution.w) ||
            (format.fmt.pix.height != maxCaptureResolution.h) ||
            (format.fmt.pix.pixelformat != pixel_format_to_v4l_pixel_format(CAPTURE_PIXEL_FORMAT)))
        {
            NBENE(("Failed to initialize the current capture format for enqueuing capture buffers (error %d).", errno));
            goto fail;
        }
    }

    // Tell the capture device that we've allocated frame buffers for it to use.
    {
        v4l2_requestbuffers buf;

        memset(&buf, 0, sizeof(buf));
        buf.count = CAPTURE_BACK_BUFFER.numPages;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;

        if (!capture_apicall(VIDIOC_REQBUFS, &buf))
        {
            NBENE(("User pointer streaming couldn't be initialized (error %d).", errno));
            goto fail;
        }
    }

    // Enqueue the frame buffers we've allocated.
    for (unsigned i = 0; i < CAPTURE_BACK_BUFFER.numPages; i++)
    {
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.index = i;
        buf.m.userptr = (unsigned long)CAPTURE_BACK_BUFFER.page(i).ptr();
        buf.length = CAPTURE_BACK_BUFFER.page(i).size();

        if (!capture_apicall(VIDIOC_QBUF, &buf))
        {
            NBENE(("Failed to enqueue capture buffers (failed on buffer #%d).", (i + 1)));
            goto fail;
        }
    }

    return true;

    fail:
    return false;
}

// Runs in a separate thread to poll for capture events from the capture device.
// The thread will be terminated externally when the program exits.
static void capture_function(capture_api_video4linux_s *const thisPtr)
{
    while (!PROGRAM_EXIT_REQUESTED)
    {
        // See if aspects of the signal have changed.
        /// TODO: Are there signal events we could hook onto, rather than
        ///       constantly polling for these parameters?
        {
            v4l2_format format = {};
            format.type = V4L2_BUF_TYPE_CAPTURE_SOURCE;

            /// TODO: Check for 'no signal' status.

            if (ioctl(CAPTURE_HANDLE, RGB133_VIDIOC_G_SRC_FMT, &format) >= 0)
            {
                const refresh_rate_s currentRefreshRate = refresh_rate_s(format.fmt.pix.priv / 1000.0);

                if ((currentRefreshRate != REFRESH_RATE) ||
                    (format.fmt.pix.width != CAPTURE_RESOLUTION.w) ||
                    (format.fmt.pix.height != CAPTURE_RESOLUTION.h))
                {
                    NO_SIGNAL = false;
                    REFRESH_RATE = currentRefreshRate;

                    push_capture_event(capture_event_e::new_video_mode);
                }
            }
            else
            {
                push_capture_event(capture_event_e::unrecoverable_error);

                break;
            }
        }

        if (thisPtr->has_no_signal())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            continue;
        }

        // Poll the capture device for a new frame.
        {
            pollfd fd;
            memset(&fd, 0, sizeof(fd));
            fd.fd = CAPTURE_HANDLE;
            fd.events = (POLLIN | POLLPRI);

            const int pollResult = poll(&fd, 1, 1000);

            // Received a new frame.
            if (pollResult > 0)
            {
                if (!(fd.revents & (POLLIN | POLLPRI)))
                {
                    continue;
                }

                v4l2_buffer buf;
                memset(&buf, 0, sizeof(buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_USERPTR;

                // Tell the capture device we want to access its capture buffer.
                if (!capture_apicall(VIDIOC_DQBUF, &buf))
                {
                    switch (errno)
                    {
                        case EAGAIN: continue;
                        default: push_capture_event(capture_event_e::unrecoverable_error); break;
                    }
                }

                // If the hardware is sending us a new frame while we're still unfinished
                // processing the previous frame, we'll skip this new frame.
                if (NUM_FRAMES_CAPTURED != NUM_FRAMES_PROCESSED)
                {
                    NUM_NEW_FRAME_EVENTS_SKIPPED++;
                }
                else
                {
                    std::lock_guard<std::mutex> lock(thisPtr->captureMutex);

                    FRAME_BUFFER.r = CAPTURE_RESOLUTION;
                    FRAME_BUFFER.r.bpp = ((CAPTURE_PIXEL_FORMAT == capture_pixel_format_e::rgb_888)? 32 : 16);
                    FRAME_BUFFER.pixelFormat = CAPTURE_PIXEL_FORMAT;

                    // Copy the frame's data into our local buffer so we can work on it.
                    memcpy(FRAME_BUFFER.pixels.ptr(), (char*)buf.m.userptr,
                           FRAME_BUFFER.pixels.up_to(FRAME_BUFFER.r.w * FRAME_BUFFER.r.h * (FRAME_BUFFER.r.bpp / 8)));

                    NUM_FRAMES_CAPTURED++;
                    push_capture_event(capture_event_e::new_frame);
                }

                // Tell the capture device it can use its capture buffer again.
                if (!capture_apicall(VIDIOC_QBUF, &buf))
                {
                    push_capture_event(capture_event_e::unrecoverable_error);
                    break;
                }
            }
            // A capture error.
            else
            {
                std::lock_guard<std::mutex> lock(thisPtr->captureMutex);

                push_capture_event(capture_event_e::unrecoverable_error);
            }
        }
    }

    return;
}

capture_event_e capture_api_video4linux_s::pop_capture_event_queue(void)
{
    if (pop_capture_event(capture_event_e::unrecoverable_error))
    {
        return capture_event_e::unrecoverable_error;
    }
    else if (pop_capture_event(capture_event_e::new_video_mode))
    {
        this->set_resolution(this->get_source_resolution());

        SIGNAL_CONTROLS.update();

        return capture_event_e::new_video_mode;
    }
    else if (pop_capture_event(capture_event_e::signal_lost))
    {
        SIGNAL_CONTROLS.update();

        return capture_event_e::signal_lost;
    }
    else if (pop_capture_event(capture_event_e::invalid_signal))
    {
        return capture_event_e::invalid_signal;
    }
    else if (pop_capture_event(capture_event_e::new_frame))
    {
        return capture_event_e::new_frame;
    }

    // If there were no events we should notify the caller about.
    return (this->has_no_signal()? capture_event_e::sleep : capture_event_e::none);
}

resolution_s capture_api_video4linux_s::get_resolution(void) const
{
    return CAPTURE_RESOLUTION;
}

resolution_s capture_api_video4linux_s::get_minimum_resolution(void) const
{
    /// TODO: Query actual hardware parameters for this.
    return resolution_s{MIN_OUTPUT_WIDTH, MIN_OUTPUT_HEIGHT, 32};
}

resolution_s capture_api_video4linux_s::get_maximum_resolution(void) const
{
    /// TODO: Query actual hardware parameters for this.
    return resolution_s{1920, 1080, 32};
}

refresh_rate_s capture_api_video4linux_s::get_refresh_rate(void) const
{
    return REFRESH_RATE;
}

uint capture_api_video4linux_s::get_missed_frames_count(void) const
{
    return NUM_NEW_FRAME_EVENTS_SKIPPED;
}

bool capture_api_video4linux_s::has_invalid_signal() const
{
    return INVALID_SIGNAL;
}

bool capture_api_video4linux_s::has_no_signal() const
{
    return NO_SIGNAL;
}

capture_pixel_format_e capture_api_video4linux_s::get_pixel_format() const
{
    return CAPTURE_PIXEL_FORMAT;
}

resolution_s capture_api_video4linux_s::get_source_resolution(void) const
{
    v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_CAPTURE_SOURCE;

    if (!capture_apicall(RGB133_VIDIOC_G_SRC_FMT, &format))
    {
        k_assert(0, "The capture hardware failed to report its input resolution.");
    }

    return {format.fmt.pix.width,
            format.fmt.pix.height,
            32}; /// TODO: Don't assume the bit depth.
}

bool capture_api_video4linux_s::initialize_hardware(void)
{
    // Open the capture device.
    /// TODO: Query for the correct video channel rather than hardcoding /dev/video0.
    if ((CAPTURE_HANDLE = open((std::string("/dev/video") + std::to_string(INPUT_CHANNEL_IDX)).c_str(), O_RDWR)) < 0)
    {
        NBENE(("Failed to open the capture device."));

        goto fail;
    }

    // Verify device capabilities.
    {
        v4l2_capability caps = {};

        if (!capture_apicall(VIDIOC_QUERYCAP, &caps))
        {
            NBENE(("Failed to query capture device capabilities."));

            goto fail;
        }

        if (!(caps.capabilities & V4L2_CAP_READWRITE))
        {
            NBENE(("The capture device doesn't support read/write - can't do capture."));

            goto fail;
        }
    }

    if (!this->enqueue_capture_buffers())
    {
        NBENE(("Failed to enqueue the capture buffers."));

        goto fail;
    }

    // Set the starting capture resolution.
    {
        const resolution_s sourceResolution = this->get_source_resolution();

        v4l2_format format = {};
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (!capture_apicall(VIDIOC_G_FMT, &format))
        {
            NBENE(("Failed to query the current capture format (error %d).", errno));
            goto fail;
        }

        format.fmt.pix.width = sourceResolution.w;
        format.fmt.pix.height = sourceResolution.h;
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;
        format.fmt.pix.field = V4L2_FIELD_NONE;

        if (!capture_apicall(VIDIOC_S_FMT, &format))
        {
            NBENE(("Failed to query the current capture format (error %d).", errno));
            goto fail;
        }

        if (!capture_apicall(VIDIOC_G_FMT, &format))
        {
            NBENE(("Failed to query the current capture format (error %d).", errno));
            goto fail;
        }

        if (format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB32)
        {
            NBENE(("Failed to initialize the correct capture pixel format.", errno));
            goto fail;
        }

        CAPTURE_RESOLUTION.w = format.fmt.pix.width;
        CAPTURE_RESOLUTION.h = format.fmt.pix.height;
        CAPTURE_RESOLUTION.bpp = 32;
    }

    // Start capture.
    if (!stream_on())
    {
        goto fail;
    }

    return true;

    fail:
    NBENE(("Failed to initialize the capture device."));
    return false;
}

bool capture_api_video4linux_s::set_resolution(const resolution_s &r)
{
    v4l2_format format = {};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (!capture_apicall(VIDIOC_G_FMT, &format))
    {
        NBENE(("Failed to query the current capture format (error %d).", errno));

        goto fail;
    }

    format.fmt.pix.width = r.w;
    format.fmt.pix.height = r.h;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;

    if (!capture_apicall(VIDIOC_S_FMT, &format) ||
        !capture_apicall(VIDIOC_G_FMT, &format))
    {
        NBENE(("Failed to set the current capture format (error %d).", errno));

        goto fail;
    }

    k_assert((format.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB32),
             "Invalid capture pixel format.");

    CAPTURE_RESOLUTION.w = format.fmt.pix.width;
    CAPTURE_RESOLUTION.h = format.fmt.pix.height;
    CAPTURE_RESOLUTION.bpp = 32;

    return true;

    fail:
    return false;
}

bool capture_api_video4linux_s::release_hardware(void)
{
    bool successFlag = true;

    if (!stream_off())
    {
        successFlag = false;
    }

    return successFlag;
}

bool capture_api_video4linux_s::initialize(void)
{
    FRAME_BUFFER.r = {640, 480, 32};
    FRAME_BUFFER.pixelFormat = capture_pixel_format_e::rgb_888;
    FRAME_BUFFER.pixels.alloc(MAX_FRAME_SIZE);

    CAPTURE_BACK_BUFFER.allocate();

    if (!this->initialize_hardware())
    {
        goto fail;
    }

    // Start the capture thread.
    std::thread(capture_function, this).detach();

    return true;

    fail:
    PROGRAM_EXIT_REQUESTED = 1;
    return false;
}

bool capture_api_video4linux_s::release(void)
{
    this->release_hardware();
    close(CAPTURE_HANDLE);

    FRAME_BUFFER.pixels.release_memory();
    CAPTURE_BACK_BUFFER.release();

    return true;
}

std::string capture_api_video4linux_s::get_device_name(void) const
{
    v4l2_capability caps = {};

    if (!capture_apicall(VIDIOC_QUERYCAP, &caps))
    {
        NBENE(("Failed to query capture device capabilities."));

        return "Unknown";
    }

    return (char*)caps.card;
}

std::string capture_api_video4linux_s::get_device_driver_version() const
{
    v4l2_capability caps = {};

    if (!capture_apicall(VIDIOC_QUERYCAP, &caps))
    {
        NBENE(("Failed to query capture device capabilities."));

        return "Unknown";
    }

    return (std::string((char*)caps.driver)             + " " +
            std::to_string((caps.version >> 16) & 0xff) + "." +
            std::to_string((caps.version >> 8) & 0xff)  + "." +
            std::to_string((caps.version >> 0) & 0xff));
}

std::string capture_api_video4linux_s::get_device_firmware_version(void) const
{
    return "Unknown";
}


int capture_api_video4linux_s::get_device_maximum_input_count(void) const
{
    std::string video_device_prefix = "/dev/video";
    int total_devices = 0;

    for (int i = 0; i < 64; i++) 
    {
        v4l2_capability card_caps; 

        int f = open((video_device_prefix + std::to_string(i)).c_str(), O_RDONLY);
        
        if (f < 0)
            continue;

        if (ioctl(f, VIDIOC_QUERYCAP, &card_caps) >= 0 && is_vision_caps(&card_caps))
            total_devices++;

        close(f);
    }
    
    return total_devices;
}

video_signal_parameters_s capture_api_video4linux_s::get_video_signal_parameters(void) const
{
    if (this->has_no_signal())
    {
        return this->get_default_video_signal_parameters();
    }

    video_signal_parameters_s p;

    p.phase              = SIGNAL_CONTROLS.value(signal_parameters_s::parameter_type_e::phase);
    p.blackLevel         = SIGNAL_CONTROLS.value(signal_parameters_s::parameter_type_e::black_level);
    p.horizontalPosition = SIGNAL_CONTROLS.value(signal_parameters_s::parameter_type_e::horizontal_position);
    p.verticalPosition   = SIGNAL_CONTROLS.value(signal_parameters_s::parameter_type_e::vertical_position);
    p.horizontalScale    = SIGNAL_CONTROLS.value(signal_parameters_s::parameter_type_e::horizontal_size);
    p.overallBrightness  = SIGNAL_CONTROLS.value(signal_parameters_s::parameter_type_e::brightness);
    p.overallContrast    = SIGNAL_CONTROLS.value(signal_parameters_s::parameter_type_e::contrast);
    p.redBrightness      = SIGNAL_CONTROLS.value(signal_parameters_s::parameter_type_e::red_brightness);
    p.greenBrightness    = SIGNAL_CONTROLS.value(signal_parameters_s::parameter_type_e::green_brightness);
    p.blueBrightness     = SIGNAL_CONTROLS.value(signal_parameters_s::parameter_type_e::blue_brightness);
    p.redContrast        = SIGNAL_CONTROLS.value(signal_parameters_s::parameter_type_e::red_contrast);
    p.greenContrast      = SIGNAL_CONTROLS.value(signal_parameters_s::parameter_type_e::green_contrast);
    p.blueContrast       = SIGNAL_CONTROLS.value(signal_parameters_s::parameter_type_e::blue_contrast);

    return p;
}

video_signal_parameters_s capture_api_video4linux_s::get_default_video_signal_parameters(void) const
{
    video_signal_parameters_s p;

    // The V4L API returns no parameter ranges while there's no signal - so let's
    // approximate them.
    if (this->has_no_signal())
    {
        p.phase              = 0;
        p.blackLevel         = 8;
        p.horizontalPosition = 112;
        p.verticalPosition   = 11;
        p.horizontalScale    = 800;
        p.overallBrightness  = 32;
        p.overallContrast    = 128;
        p.redBrightness      = 128;
        p.greenBrightness    = 256;
        p.blueBrightness     = 128;
        p.redContrast        = 256;
        p.greenContrast      = 128;
        p.blueContrast       = 256;
    }
    else
    {
        p.phase              = SIGNAL_CONTROLS.default_value(signal_parameters_s::parameter_type_e::phase);
        p.blackLevel         = SIGNAL_CONTROLS.default_value(signal_parameters_s::parameter_type_e::black_level);
        p.horizontalPosition = SIGNAL_CONTROLS.default_value(signal_parameters_s::parameter_type_e::horizontal_position);
        p.verticalPosition   = SIGNAL_CONTROLS.default_value(signal_parameters_s::parameter_type_e::vertical_position);
        p.horizontalScale    = SIGNAL_CONTROLS.default_value(signal_parameters_s::parameter_type_e::horizontal_size);
        p.overallBrightness  = SIGNAL_CONTROLS.default_value(signal_parameters_s::parameter_type_e::brightness);
        p.overallContrast    = SIGNAL_CONTROLS.default_value(signal_parameters_s::parameter_type_e::contrast);
        p.redBrightness      = SIGNAL_CONTROLS.default_value(signal_parameters_s::parameter_type_e::red_brightness);
        p.greenBrightness    = SIGNAL_CONTROLS.default_value(signal_parameters_s::parameter_type_e::green_brightness);
        p.blueBrightness     = SIGNAL_CONTROLS.default_value(signal_parameters_s::parameter_type_e::blue_brightness);
        p.redContrast        = SIGNAL_CONTROLS.default_value(signal_parameters_s::parameter_type_e::red_contrast);
        p.greenContrast      = SIGNAL_CONTROLS.default_value(signal_parameters_s::parameter_type_e::green_contrast);
        p.blueContrast       = SIGNAL_CONTROLS.default_value(signal_parameters_s::parameter_type_e::blue_contrast);
    }

    return p;
}

video_signal_parameters_s capture_api_video4linux_s::get_minimum_video_signal_parameters(void) const
{
    video_signal_parameters_s p;

    // The V4L API returns no parameter ranges while there's no signal - so let's
    // approximate them.
    if (this->has_no_signal())
    {
        p.phase              = 0;
        p.blackLevel         = 1;
        p.horizontalPosition = 32;
        p.verticalPosition   = 4;
        p.horizontalScale    = 100;
        p.overallBrightness  = 0;
        p.overallContrast    = 0;
        p.redBrightness      = 0;
        p.greenBrightness    = 0;
        p.blueBrightness     = 0;
        p.redContrast        = 0;
        p.greenContrast      = 0;
        p.blueContrast       = 0;
    }
    else
    {
        p.phase              = SIGNAL_CONTROLS.minimum_value(signal_parameters_s::parameter_type_e::phase);
        p.blackLevel         = SIGNAL_CONTROLS.minimum_value(signal_parameters_s::parameter_type_e::black_level);
        p.horizontalPosition = SIGNAL_CONTROLS.minimum_value(signal_parameters_s::parameter_type_e::horizontal_position);
        p.verticalPosition   = SIGNAL_CONTROLS.minimum_value(signal_parameters_s::parameter_type_e::vertical_position);
        p.horizontalScale    = SIGNAL_CONTROLS.minimum_value(signal_parameters_s::parameter_type_e::horizontal_size);
        p.overallBrightness  = SIGNAL_CONTROLS.minimum_value(signal_parameters_s::parameter_type_e::brightness);
        p.overallContrast    = SIGNAL_CONTROLS.minimum_value(signal_parameters_s::parameter_type_e::contrast);
        p.redBrightness      = SIGNAL_CONTROLS.minimum_value(signal_parameters_s::parameter_type_e::red_brightness);
        p.greenBrightness    = SIGNAL_CONTROLS.minimum_value(signal_parameters_s::parameter_type_e::green_brightness);
        p.blueBrightness     = SIGNAL_CONTROLS.minimum_value(signal_parameters_s::parameter_type_e::blue_brightness);
        p.redContrast        = SIGNAL_CONTROLS.minimum_value(signal_parameters_s::parameter_type_e::red_contrast);
        p.greenContrast      = SIGNAL_CONTROLS.minimum_value(signal_parameters_s::parameter_type_e::green_contrast);
        p.blueContrast       = SIGNAL_CONTROLS.minimum_value(signal_parameters_s::parameter_type_e::blue_contrast);
    }

    return p;
}

video_signal_parameters_s capture_api_video4linux_s::get_maximum_video_signal_parameters(void) const
{
    video_signal_parameters_s p;

    // The V4L API returns no parameter ranges while there's no signal - so let's
    // approximate them.
    if (this->has_no_signal())
    {
        p.phase              = 31;
        p.blackLevel         = 255;
        p.horizontalPosition = 800;
        p.verticalPosition   = 15;
        p.horizontalScale    = 4095;
        p.overallBrightness  = 255;
        p.overallContrast    = 255;
        p.redBrightness      = 255;
        p.greenBrightness    = 511;
        p.blueBrightness     = 255;
        p.redContrast        = 511;
        p.greenContrast      = 255;
        p.blueContrast       = 511;
    }
    else
    {
        p.phase              = SIGNAL_CONTROLS.maximum_value(signal_parameters_s::parameter_type_e::phase);
        p.blackLevel         = SIGNAL_CONTROLS.maximum_value(signal_parameters_s::parameter_type_e::black_level);
        p.horizontalPosition = SIGNAL_CONTROLS.maximum_value(signal_parameters_s::parameter_type_e::horizontal_position);
        p.verticalPosition   = SIGNAL_CONTROLS.maximum_value(signal_parameters_s::parameter_type_e::vertical_position);
        p.horizontalScale    = SIGNAL_CONTROLS.maximum_value(signal_parameters_s::parameter_type_e::horizontal_size);
        p.overallBrightness  = SIGNAL_CONTROLS.maximum_value(signal_parameters_s::parameter_type_e::brightness);
        p.overallContrast    = SIGNAL_CONTROLS.maximum_value(signal_parameters_s::parameter_type_e::contrast);
        p.redBrightness      = SIGNAL_CONTROLS.maximum_value(signal_parameters_s::parameter_type_e::red_brightness);
        p.greenBrightness    = SIGNAL_CONTROLS.maximum_value(signal_parameters_s::parameter_type_e::green_brightness);
        p.blueBrightness     = SIGNAL_CONTROLS.maximum_value(signal_parameters_s::parameter_type_e::blue_brightness);
        p.redContrast        = SIGNAL_CONTROLS.maximum_value(signal_parameters_s::parameter_type_e::red_contrast);
        p.greenContrast      = SIGNAL_CONTROLS.maximum_value(signal_parameters_s::parameter_type_e::green_contrast);
        p.blueContrast       = SIGNAL_CONTROLS.maximum_value(signal_parameters_s::parameter_type_e::blue_contrast);
    }

    return p;
}

const captured_frame_s& capture_api_video4linux_s::get_frame_buffer(void) const
{
    return FRAME_BUFFER;
}

bool capture_api_video4linux_s::mark_frame_buffer_as_processed(void)
{
    NUM_FRAMES_PROCESSED = NUM_FRAMES_CAPTURED.load();

    FRAME_BUFFER.processed = true;

    return true;
}

bool capture_api_video4linux_s::reset_missed_frames_count()
{
    NUM_NEW_FRAME_EVENTS_SKIPPED = 0;

    return true;
}

bool capture_api_video4linux_s::set_video_signal_parameters(const video_signal_parameters_s &p)
{
    if (this->has_no_signal())
    {
        DEBUG(("Was asked to set capture video params while there was no signal. "
               "Ignoring the request."));

        return true;
    }

    const auto set_parameter = [](const int value, const signal_parameters_s::parameter_type_e parameterType)
    {
        if (SIGNAL_CONTROLS.value(parameterType) != value)
        {
            return SIGNAL_CONTROLS.set_value(value, parameterType);
        }

        return true;
    };

    set_parameter(p.phase,              signal_parameters_s::parameter_type_e::phase);
    set_parameter(p.blackLevel,         signal_parameters_s::parameter_type_e::black_level);
    set_parameter(p.horizontalPosition, signal_parameters_s::parameter_type_e::horizontal_position);
    set_parameter(p.verticalPosition,   signal_parameters_s::parameter_type_e::vertical_position);
    set_parameter(p.horizontalScale,    signal_parameters_s::parameter_type_e::horizontal_size);
    set_parameter(p.overallBrightness,  signal_parameters_s::parameter_type_e::brightness);
    set_parameter(p.overallContrast,    signal_parameters_s::parameter_type_e::contrast);
    set_parameter(p.redBrightness,      signal_parameters_s::parameter_type_e::red_brightness);
    set_parameter(p.greenBrightness,    signal_parameters_s::parameter_type_e::green_brightness);
    set_parameter(p.blueBrightness,     signal_parameters_s::parameter_type_e::blue_brightness);
    set_parameter(p.redContrast,        signal_parameters_s::parameter_type_e::red_contrast);
    set_parameter(p.greenContrast,      signal_parameters_s::parameter_type_e::green_contrast);
    set_parameter(p.blueContrast,       signal_parameters_s::parameter_type_e::blue_contrast);

    SIGNAL_CONTROLS.update();

    return true;
}

#endif
