/*
 * 2018 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Handles interactions with the capture hardware.
 *
 */

#include "capture/capture.h"
#include "common/timer/timer.h"

static std::mutex CAPTURE_MUTEX;

std::mutex& kc_mutex(void)
{
    return CAPTURE_MUTEX;
}

subsystem_releaser_t kc_initialize_capture(void)
{
    DEBUG(("Initializing the capture subsystem."));

    kc_initialize_device();

    return []{
        DEBUG(("Releasing the capture subsystem."));
        kc_release_device();
    };
}

bool kc_has_signal(void)
{
    return kc_device_property("has signal");
}

video_signal_properties_s video_signal_properties_s::from_capture_device_properties(const std::string &nameSpace)
{
    return video_signal_properties_s{
        .brightness         = long(kc_device_property("brightness" + nameSpace)),
        .contrast           = long(kc_device_property("contrast" + nameSpace)),
        .redBrightness      = long(kc_device_property("red brightness" + nameSpace)),
        .redContrast        = long(kc_device_property("red contrast" + nameSpace)),
        .greenBrightness    = long(kc_device_property("green brightness" + nameSpace)),
        .greenContrast      = long(kc_device_property("green contrast" + nameSpace)),
        .blueBrightness     = long(kc_device_property("blue brightness" + nameSpace)),
        .blueContrast       = long(kc_device_property("blue contrast" + nameSpace)),
        .horizontalSize     = ulong(kc_device_property("horizontal size" + nameSpace)),
        .horizontalPosition = long(kc_device_property("horizontal position" + nameSpace)),
        .verticalPosition   = long(kc_device_property("vertical position" + nameSpace)),
        .phase              = long(kc_device_property("phase" + nameSpace)),
        .blackLevel         = long(kc_device_property("black level" + nameSpace))
    };
}

void video_signal_properties_s::to_capture_device_properties(const video_signal_properties_s &params, const std::string &nameSpace)
{
    kc_set_device_property(("brightness" + nameSpace), params.brightness);
    kc_set_device_property(("contrast" + nameSpace), params.contrast);
    kc_set_device_property(("red brightness" + nameSpace), params.redBrightness);
    kc_set_device_property(("green brightness" + nameSpace), params.greenBrightness);
    kc_set_device_property(("blue brightness" + nameSpace), params.blueBrightness);
    kc_set_device_property(("red contrast" + nameSpace), params.redContrast);
    kc_set_device_property(("green contrast" + nameSpace), params.greenContrast);
    kc_set_device_property(("blue contrast" + nameSpace), params.blueContrast);
    kc_set_device_property(("horizontal size" + nameSpace), params.horizontalSize);
    kc_set_device_property(("horizontal position" + nameSpace), params.horizontalPosition);
    kc_set_device_property(("vertical position" + nameSpace), params.verticalPosition);
    kc_set_device_property(("phase" + nameSpace), params.phase);
    kc_set_device_property(("black level" + nameSpace), params.blackLevel);

    return;
}
