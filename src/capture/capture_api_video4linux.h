/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifdef CAPTURE_API_VIDEO4LINUX

#ifndef CAPTURE_API_VIDEO4LINUX_H
#define CAPTURE_API_VIDEO4LINUX_H

#include "capture/capture_api.h"

struct capture_api_video4linux_s : public capture_api_s
{
    bool initialize(void) override;
    bool release(void) override;

    std::string get_device_name(void) const override;
    std::string get_api_name(void) const override                { return "Video4Linux"; }
    std::string get_device_driver_version(void) const override;
    std::string get_device_firmware_version(void) const override;
    int get_device_maximum_input_count(void) const override;
    video_signal_parameters_s get_video_signal_parameters(void) const override;
    video_signal_parameters_s get_default_video_signal_parameters(void) const override;
    video_signal_parameters_s get_minimum_video_signal_parameters(void) const override;
    video_signal_parameters_s get_maximum_video_signal_parameters(void) const override;
    resolution_s get_resolution(void) const override;
    resolution_s get_minimum_resolution(void) const override;
    resolution_s get_maximum_resolution(void) const override;
    refresh_rate_s get_refresh_rate(void) const override;
    uint get_missed_frames_count(void) const override;
    uint get_input_channel_idx(void) const override              { return 0;  }
    uint get_color_depth(void) const override                    { return 32; }
    bool is_capturing(void) const override                       { return true; }
    bool has_invalid_signal(void) const override;
    bool has_no_signal(void) const override;
    capture_pixel_format_e get_pixel_format(void) const override;
    const captured_frame_s& get_frame_buffer(void) const override;

    capture_event_e pop_capture_event_queue(void) override;
    bool set_resolution(const resolution_s &r) override;
    bool mark_frame_buffer_as_processed(void) override;
    bool reset_missed_frames_count(void) override;
    bool set_video_signal_parameters(const video_signal_parameters_s &p) override;

    /// TODO: Properly implement these.
    bool set_input_channel(const unsigned idx) override { (void)idx; return false; }
    bool set_pixel_format(const capture_pixel_format_e pf) override { (void)pf; return false;    }

    /// TODO: Does the Vision API allow you to query these?
    bool device_supports_component_capture(void) const override { return false; }
    bool device_supports_composite_capture(void) const override { return false; }
    bool device_supports_deinterlacing(void)     const override { return false; }
    bool device_supports_svideo(void)            const override { return false; }
    bool device_supports_dma(void)               const override { return false; }
    bool device_supports_dvi(void)               const override { return false; }
    bool device_supports_vga(void)               const override { return false; }
    bool device_supports_yuv(void)               const override { return false; }

private:
    // Set up the capture device's capture buffers.
    bool enqueue_capture_buffers(void);

    // Release the capture device's capture buffers.
    bool unqueue_capture_buffers(void);

    // Prepares the capture device for capturing. Returns true on success; false
    // otherwise.
    bool initialize_hardware(void);

    // Stops the capture. Returns true on success; false otherwise.
    bool release_hardware(void);

    // Polls for the source signal resolution, rather than the capture output's
    // resolution like get_resolution().
    resolution_s get_source_resolution(void) const;
};

#endif

#endif
