/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <thread>
#include <linux/videodev2.h>
#include "capture/vision_v4l/input_channel_v4l.h"
#include "capture/vision_v4l/ic_v4l_video_parameters.h"

#define INCLUDE_VISION
#include <visionrgb/include/rgb133control.h>
#include <visionrgb/include/rgb133v4l2.h>

// We'll persist the resolution and refresh rate, so that when a new video mode
// is encountered, the code will first save the new mode values and then close
// the old input channel, with the new input channel adopting the persisted values.
static resolution_s LATEST_RESOLUTION = {1024, 768, 32};
static refresh_rate_s LATEST_REFRESH_RATE = 0;

input_channel_v4l_c::input_channel_v4l_c(const std::string v4lDeviceFileName,
                                         const unsigned numBackBuffers,
                                         captured_frame_s *const dstFrameBuffer) :
    v4lDeviceFileName(v4lDeviceFileName),
    dstFrameBuffer(dstFrameBuffer),
    requestedNumBackBuffers(numBackBuffers)
{
    DEBUG(("Opening %s.", this->v4lDeviceFileName.c_str()));

    this->captureStatus.refreshRate = LATEST_REFRESH_RATE;
    this->captureStatus.resolution = LATEST_RESOLUTION;

    this->start_capturing();

    return;
}

input_channel_v4l_c::~input_channel_v4l_c()
{
    DEBUG(("Closing %s.", this->v4lDeviceFileName.c_str()));

    const int retVal = this->stop_capturing();

    /// TODO: Do something with retVal if need be, e.g. report errors.
    (void)retVal;

    return;
}

void input_channel_v4l_c::push_capture_event(capture_event_e flag)
{
    const unsigned flagIdx = static_cast<unsigned>(flag);

    k_assert((flagIdx < this->captureEventFlags.size()), "Overflowing the capture event flag buffer.");

    this->captureEventFlags[flagIdx] = 1;

    return;
}

bool input_channel_v4l_c::pop_capture_event(const capture_event_e flag)
{
    const unsigned flagIdx = static_cast<unsigned>(flag);

    k_assert((flagIdx < this->captureEventFlags.size()), "Overflowing the capture event flag buffer.");

    const bool value = this->captureEventFlags[flagIdx];
    this->captureEventFlags[flagIdx] = 0;

    if ((flag == capture_event_e::sleep) &&
        !this->run)
    {
        return true;
    }
    else
    {
        return value;
    }
}

resolution_s input_channel_v4l_c::maximum_resolution(void) const
{
    /// TODO: Query actual hardware parameters for this.
    
    return resolution_s{std::min(2048u, MAX_CAPTURE_WIDTH),
                        std::min(1536u, MAX_CAPTURE_HEIGHT),
                        std::min(32u, MAX_CAPTURE_BPP)};
}

resolution_s input_channel_v4l_c::source_resolution(void)
{
    v4l2_format format = {0};

    format.type = V4L2_BUF_TYPE_CAPTURE_SOURCE;

    if (!this->device_ioctl(RGB133_VIDIOC_G_SRC_FMT, &format))
    {
        k_assert(0, "The capture hardware failed to report its input resolution.");
    }

    return {format.fmt.pix.width,
            format.fmt.pix.height,
            32}; /// TODO: Don't assume the bit depth.
}

bool input_channel_v4l_c::capture_thread__has_signal(void)
{
    /// FIXME: We're only assuming this is the correct ID for the V4L "signal_type" control.
    static const unsigned v4lSignalTypeControlId = 0x8000013;

    /// FIXME: We're only assuming this value of the "signal_type" control means "no signal".
    static const unsigned noSignalControlValue = 0;

    v4l2_control v4lc = {};
    v4lc.id = v4lSignalTypeControlId;

    if (ioctl(this->v4lDeviceFileHandle, VIDIOC_G_CTRL, &v4lc) == 0)
    {
        const bool hasNoSignal = (v4lc.value == noSignalControlValue);
        const bool hasStatusChanged = (hasNoSignal != this->captureStatus.noSignal);

        this->captureStatus.noSignal = hasNoSignal;

        if (hasStatusChanged)
        {
            std::lock_guard<std::mutex> lock(kc_capture_mutex());
            this->push_capture_event(hasNoSignal? capture_event_e::signal_lost : capture_event_e::signal_gained);
        }
    }

    return !this->captureStatus.noSignal;
}

bool input_channel_v4l_c::capture_thread__has_source_mode_changed(void)
{
    v4l2_format format = {0};
    format.type = V4L2_BUF_TYPE_CAPTURE_SOURCE;

    if (ioctl(this->v4lDeviceFileHandle, RGB133_VIDIOC_G_SRC_FMT, &format) >= 0)
    {
        if (!input_channel_v4l_c::is_format_of_valid_signal(&format))
        {
            std::lock_guard<std::mutex> lock(kc_capture_mutex());

            this->captureStatus.invalidSignal = true;
            this->push_capture_event(capture_event_e::invalid_signal);

            return true;
        }
        else
        {
            this->captureStatus.invalidSignal = false;
        }

        // Attempt to detect if aspects of the signal have changed.
        {
            const refresh_rate_s currentRefreshRate = refresh_rate_s(format.fmt.pix.priv / 1000.0);

            if ((currentRefreshRate != LATEST_REFRESH_RATE) ||
                (format.fmt.pix.width != LATEST_RESOLUTION.w) ||
                (format.fmt.pix.height != LATEST_RESOLUTION.h))
            {
                this->captureStatus.refreshRate = LATEST_REFRESH_RATE = currentRefreshRate;
                this->captureStatus.resolution = LATEST_RESOLUTION = {format.fmt.pix.width, format.fmt.pix.height, 32};
                return true;
            }
        }
    }
    // Failed to query the video format.
    else
    {
        std::lock_guard<std::mutex> lock(kc_capture_mutex());

        this->captureStatus.invalidSignal = true;
        this->push_capture_event(capture_event_e::unrecoverable_error);

        return true;
    }

    return false;
}

// Returns false on error; true otherwise (e.g. if we got a new frame or if there
// was no new frame to get).
bool input_channel_v4l_c::capture_thread__get_next_frame(void)
{
    pollfd fd;
    memset(&fd, 0, sizeof(fd));
    fd.fd = this->v4lDeviceFileHandle;
    fd.events = (POLLIN | POLLPRI);

    const int pollResult = poll(&fd, 1, 1000);

    // Received a new frame.
    if (pollResult > 0)
    {
        if (!(fd.revents & (POLLIN | POLLPRI)))
        {
            return true;
        }

        v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        // Tell the capture device we want to access the frame buffer's data.
        if (!this->device_ioctl(VIDIOC_DQBUF, &buf))
        {
            switch (errno)
            {
                case EAGAIN:
                {
                    return true;
                }
                default:
                {
                    std::lock_guard<std::mutex> lock(kc_capture_mutex());

                    this->push_capture_event(capture_event_e::unrecoverable_error);

                    return false;
                }
            }
        }

        // If the hardware is sending us a new frame while we're still unfinished
        // processing the previous frame, we'll skip this new frame.
        if (this->captureStatus.numFramesCaptured != this->captureStatus.numFramesProcessed)
        {
            this->captureStatus.numNewFrameEventsSkipped++;
        }
        else
        {
            std::lock_guard<std::mutex> lock(kc_capture_mutex());

            const input_channel_v4l_c::mmap_metadata &srcBuffer = this->mmapBackBuffers.at(buf.index);

            this->dstFrameBuffer->r = LATEST_RESOLUTION;
            this->dstFrameBuffer->r.bpp = ((this->captureStatus.pixelFormat == capture_pixel_format_e::rgb_888)? 32 : 16);
            this->dstFrameBuffer->pixelFormat = this->captureStatus.pixelFormat;

            // Copy the frame's data into our local buffer so we can work on it.
            memcpy(this->dstFrameBuffer->pixels, srcBuffer.ptr, srcBuffer.length);

            this->captureStatus.numFramesCaptured++;
            this->push_capture_event(capture_event_e::new_frame);
        }

        // Tell the capture device that we've finished accessing the buffer.
        if (!this->device_ioctl(VIDIOC_QBUF, &buf))
        {
            std::lock_guard<std::mutex> lock(kc_capture_mutex());

            this->push_capture_event(capture_event_e::unrecoverable_error);

            return false;
        }
    }
    // A capture error.
    else
    {
        std::lock_guard<std::mutex> lock(kc_capture_mutex());

        this->push_capture_event(capture_event_e::unrecoverable_error);
        
        return false;
    }

    return true;
}

bool input_channel_v4l_c::device_ioctl(const unsigned long request, void *data)
{
    if (this->v4lDeviceFileHandle < 0)
    {
        DEBUG(("Attempted to call device_ioctl() while the device file was closed. Ignoring this."));
        return false;
    }

    const int retVal = ioctl(this->v4lDeviceFileHandle, request, data);

    if (retVal < 0)
    {
        NBENE(("A call to ioctl() failed. Request: %lu, error: %d.", request, retVal));
        return false;
    }

    return true;
}

bool input_channel_v4l_c::open_device(const std::string &deviceFileName)
{
    k_assert((this->v4lDeviceFileHandle < 0),
             "Attempting to re-open a capture device before having closed it.");

    this->v4lDeviceFileHandle = open(deviceFileName.c_str(), (O_RDWR | O_NONBLOCK));

    if (this->v4lDeviceFileHandle < 0)
    {
        return false;
    }

    this->captureStatus.videoParameters = ic_v4l_device_controls_c(this->v4lDeviceFileHandle);

    return true;
}

bool input_channel_v4l_c::close_device(void)
{
    // Already closed.
    if (this->v4lDeviceFileHandle < 0)
    {
        return true;
    }

    if (close(this->v4lDeviceFileHandle) < 0)
    {
        return false;
    }

    this->v4lDeviceFileHandle = -1;

    return true;
}

int input_channel_v4l_c::capture_thread(void)
{
    k_assert((this->v4lDeviceFileHandle >= 0),
             "Attempting to start the capture thread before the device has been opened.");

    while (this->run)
    {
        if (!capture_thread__has_signal())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (capture_thread__has_source_mode_changed())
        {
            if (!this->captureStatus.invalidSignal)
            {
                std::lock_guard<std::mutex> lock(kc_capture_mutex());

                this->push_capture_event(capture_event_e::new_video_mode);

                // The parent is expected to re-spawn this input channel, so we can exit the
                // capture thread.
                return 1;
            }
            else
            {
                continue;
            }
        }

        if (!capture_thread__get_next_frame())
        {
            return 0;
        }
    }

    return 1;
}

u32 input_channel_v4l_c::vcs_pixel_format_to_v4l_pixel_format(capture_pixel_format_e fmt) const
{
    switch (fmt)
    {
        case capture_pixel_format_e::rgb_555: return V4L2_PIX_FMT_RGB555;
        case capture_pixel_format_e::rgb_565: return V4L2_PIX_FMT_RGB565;
        case capture_pixel_format_e::rgb_888: return V4L2_PIX_FMT_RGB32;
        default: k_assert(0, "Unknown pixel format."); return V4L2_PIX_FMT_RGB32;
    }
}

bool input_channel_v4l_c::set_v4l_buffer_resolution(const resolution_s &resolution)
{
    v4l2_format format = {0};

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (!this->device_ioctl(VIDIOC_G_FMT, &format))
    {
        NBENE(("Failed to query the current capture format (error %d).", errno));
        goto fail;
    }

    format.fmt.pix.width = resolution.w;
    format.fmt.pix.height = resolution.h;
    format.fmt.pix.pixelformat = this->vcs_pixel_format_to_v4l_pixel_format(this->captureStatus.pixelFormat);
    format.fmt.pix.field = V4L2_FIELD_NONE;

    if (!this->device_ioctl(VIDIOC_S_FMT, &format) ||
        !this->device_ioctl(VIDIOC_G_FMT, &format))
    {
        NBENE(("Failed to set the capture resolution (error %d).", errno));
        goto fail;
    }

    if ((format.fmt.pix.width != resolution.w) ||
        (format.fmt.pix.height != resolution.h) ||
        (format.fmt.pix.pixelformat != this->vcs_pixel_format_to_v4l_pixel_format(this->captureStatus.pixelFormat)))
    {
        NBENE(("Failed to set the capture resolution (error %d).", errno));
        goto fail;
    }

    return true;

    fail:
    return false;
}

bool input_channel_v4l_c::enqueue_mmap_back_buffers(const resolution_s &resolution)
{
    v4l2_requestbuffers reqBuf = {0};

    if (!this->set_v4l_buffer_resolution(resolution))
    {
        goto fail;
    }

    // Tell the capture device we want it to allocate the back buffers in its
    // own memory and that we'll access them via mmap.
    {
        reqBuf.count = this->requestedNumBackBuffers;
        reqBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        reqBuf.memory = V4L2_MEMORY_MMAP;

        if (!this->device_ioctl(VIDIOC_REQBUFS, &reqBuf))
        {
            NBENE(("MMAP streaming couldn't be initialized (error %d).", errno));
            goto fail;
        }
    }

    // Have the capture device allocate the back buffers in its own memory.
    for (uint32_t i = 0; i < reqBuf.count; i++)
    {
        v4l2_buffer buffer = {0};
        input_channel_v4l_c::mmap_metadata bufferMetadata = {0};

        buffer.type = reqBuf.type;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = i;

        if (!this->device_ioctl(VIDIOC_QUERYBUF, &buffer))
        {
            NBENE(("Failed to allocate back buffers on the capture device (error %d).", errno));
            goto fail;
        }

        const void *mmapPtr = mmap(NULL,
                                   buffer.length,
                                   (PROT_READ | PROT_WRITE),
                                   MAP_SHARED,
                                   this->v4lDeviceFileHandle,
                                   buffer.m.offset);

        if (mmapPtr == MAP_FAILED)
        {
            NBENE(("Failed to allocate back buffers on the capture device (mmap() returned MAP_FAILED)."));
            goto fail;
        }

        buffer.flags = 0;
        buffer.reserved = 0;
        buffer.reserved2 = 0;

        if (!this->device_ioctl(VIDIOC_QBUF, &buffer))
        {
            NBENE(("Failed to enqueue back buffers (failed on buffer #%d).", (i + 1)));
            goto fail;
        }

        bufferMetadata.ptr = (uint8_t*)mmapPtr;
        bufferMetadata.length = buffer.length;

        this->mmapBackBuffers.push_back(bufferMetadata);
    }

    return true;

    fail:
    return false;
}

bool input_channel_v4l_c::streamon(void)
{
    v4l2_buf_type bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (!this->device_ioctl(VIDIOC_STREAMON, &bufType))
    {
        NBENE(("Couldn't start the capture stream for device \"%s\".",
               this->v4lDeviceFileName.c_str()));

        return false;
    }

    return true;
}

bool input_channel_v4l_c::streamoff(void)
{
    v4l2_buf_type bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (!this->device_ioctl(VIDIOC_STREAMOFF, &bufType))
    {
        DEBUG(("Couldn't stop the capture stream for device \"%s\".",
               this->v4lDeviceFileName.c_str()));

        return false;
    }

    return true;
}

bool input_channel_v4l_c::dequeue_mmap_back_buffers(void)
{
    v4l2_requestbuffers buf = {0};

    buf.count = 0; // 0 releases all buffers.
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (!this->device_ioctl(VIDIOC_REQBUFS, &buf))
    {
        NBENE(("Stream buffers could not be deallocated (error %d).", errno));
        goto fail;
    }

    for (const auto &buffer: this->mmapBackBuffers)
    {
        munmap(buffer.ptr, buffer.length);
    }

    return true;

    fail:
    return false;
}

bool input_channel_v4l_c::is_format_of_valid_signal(const v4l2_format *const format)
{
    return ((format->fmt.pix.width >= MIN_CAPTURE_WIDTH) &&
            (format->fmt.pix.width <= MAX_CAPTURE_WIDTH) &&
            (format->fmt.pix.height >= MIN_CAPTURE_HEIGHT) &&
            (format->fmt.pix.height <= MAX_CAPTURE_HEIGHT));
}

bool input_channel_v4l_c::start_capturing()
{
    if (!this->open_device(this->v4lDeviceFileName))
    {
        NBENE(("Failed to open the capture device \"%s\".",
               this->v4lDeviceFileName.c_str()));

        goto fail;
    }

    // Verify device capabilities.
    {
        v4l2_capability caps = {};

        if (!this->device_ioctl(VIDIOC_QUERYCAP, &caps))
        {
            NBENE(("Failed to query the capabilities of capture device \"%s\".",
                   this->v4lDeviceFileName.c_str()));

            goto fail;
        }

        if (!(caps.capabilities & V4L2_CAP_READWRITE) ||
            !(caps.capabilities & V4L2_CAP_STREAMING))
        {
            NBENE(("The capture device \"%s\" doesn't support the required features.",
                   this->v4lDeviceFileName.c_str()));

            goto fail;
        }
    }

    if (!this->enqueue_mmap_back_buffers(this->source_resolution()))
    {
        goto fail;
    }

    if (!this->streamon())
    {
        goto fail;
    }

    // Start the capture thread.
    {
        this->run = true;
        this->captureThreadFuture = std::async(std::launch::async,
                                               &input_channel_v4l_c::capture_thread,
                                               this);

        if (!this->captureThreadFuture.valid())
        {
            goto fail;
        }
    }

    INFO(("Capturing on %s.", this->v4lDeviceFileName.c_str()));

    return true;

    fail:
    {
        this->captureStatus.invalidDevice = true;
        this->run = false;

        if (this->captureThreadFuture.valid())
        {
            this->captureThreadFuture.wait();
        }

        this->reset_capture_event_flags();
        this->push_capture_event(capture_event_e::signal_lost);
    }
    return false;
}

void input_channel_v4l_c::reset_capture_event_flags(void)
{
    for (auto &flag: this->captureEventFlags)
    {
        flag = 0;
    }

    return;
}

int input_channel_v4l_c::stop_capturing(void)
{
    int retVal = 1;

    // The 'run' flag would be false e.g. if we were not able to start capturing
    // on this channel to begin with.
    if (this->run)
    {
        // Ask the capture thread to stop, and block until it does so.
        this->run = false;
        retVal = (this->captureThreadFuture.valid()? this->captureThreadFuture.get() : 0);

        // Stop the capture stream. Note that we don't return on error, we just
        // continue to force the capturing to stop.
        this->streamoff();

        this->dequeue_mmap_back_buffers();
    }
      
    this->reset_capture_event_flags();
    this->push_capture_event(capture_event_e::signal_lost);

    // Note: we don't return on error, we just continue to force the capturing
    // to stop.
    if (!this->close_device())
    {
        DEBUG(("The capture device \"%s\" was not correctly closed.",
               this->v4lDeviceFileName.c_str()));
    }

    return retVal;
}
