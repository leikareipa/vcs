# VCS

A control application for Datapath Vision capture cards under Linux.

Also has experimental support for generic Video4Linux devices, digital cameras via gPhoto2, and a shared memory interface for inter-process capture.

![VCS 3.0](./screenshot.png)

## Key features

- Unlimited video presets
- Various filters and scalers
- Variable refresh rate output
- Free and open source
- Modular, extendable design

## Supported capture hardware

Any model of Datapath capture card supported by the Datapath Vision driver for Linux should be compatible, although the card's full set of capabilities may not be exposed.

Other capture devices can be supported via custom [capture backends](#the-capture-backend).

## Documentation

- [User's manual](./docs/user/README.md) (follows the master branch)
- [Guides for developers](./docs/developer/)

## Building

Open [vcs.pro](vcs.pro) in Qt Creator, or run `$ qmake && make -j` in the repo's root. You'll need to meet the [dependencies](#dependencies) and may need to adjust the [build configuration](#build-configuration).

### Dependencies

The VCS codebase depends on the following libraries and frameworks:

- Qt
- OpenCV

Using this build environment should give you the least number of compatibility issues:

| Distro       | Compiler | Qt   | OpenCV     |
| ------------ | -------- | ---- | ---------- |
| Ubuntu 20.04 | GCC 9    | 5.12 | 3.x, 4.x   |

Versions of GCC newer than 9 [have been reported](https://github.com/leikareipa/vcs/issues/34) to fail to build VCS without minor modifications to the code.

Major versions of Qt other than 5 will fail to compile and/or break many aspects of GUI functionality.

VCS is currently developed against OpenCV 3.2, but OpenCV 4 appears to be equally compatible.

### Build configuration

#### Include paths

The following variables in [vcs.pro](vcs.pro) control where the build system looks for external dependencies:

- `DATAPATH_VISION_HEADER_PATH`
- `OPENCV_HEADER_PATH`
- `OPENCV_LIB_PATH`
- `OPENCV_DEBUG_LIB_PATH` (optional, for debug builds)

You'll likely need to modify at least some of them to point to the correct locations on your system.

#### Release build vs. debug build

VCS can be built in two modes: release or debug. A release build is intended for normal usage, while a debug build includes additional run-time error checks that generally reduce performance but help reveal programming errors.

To enable debug mode, define `VCS_DEBUG_BUILD` in [vcs.pro](vcs.pro). Otherwise, the build will be in release mode. You can confirm whether the program is running in release or debug mode by navigating to Control panel &rarr; About VCS.

In debug mode, you can optionally enable AddressSanitizer and UndefinedBehaviorSanitizer by defining `VCS_DEBUG_BUILD_USES_SANITIZERS` in [vcs.pro](vcs.pro). To suppress Sanitizer warnings in external libraries, create an `asan-suppress.txt` file in the build directory and populate it according to [this guide](https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer#suppressions).

#### The capture backend

A capture backend is an implementation in VCS providing support for a particular capture source, be it a hardware device (e.g. a Datapath Vision capture card) or something else.

One &ndash; and only one &ndash; capture backend must be active in any build of VCS. To select which, include the corresponding identifier in the `DEFINES` variable in [vcs.pro](vcs.pro).

The following capture backends are currently available to choose from:

| Backend identifier          | Purpose                                                                                                                                                     |
| --------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [CAPTURE_BACKEND_VISION_V4L](./src/capture/vision_v4l/)  | Supports the Datapath Vision range of capture cards in Linux. Requires installation of the Datapath Vision driver.                                          |
| [CAPTURE_BACKEND_VIRTUAL](./src/capture/virtual/)     | Provides a virtual capture device that generates a test image.                                                                                              |
| [CAPTURE_BACKEND_GENERIC_V4L](./src/capture/generic_v4l/) | A sample implementation of a generic, non-Datapath-specific capture backend using Video4Linux. For educational purposes.                                    |
| [CAPTURE_BACKEND_GPHOTO2](./src/capture/gphoto2/)     | Exposes some of the functionality of gPhoto2 to allow interaction with digital cameras. Requires libgphoto2. A lazy implementation, for hobby projects etc. |
| [CAPTURE_BACKEND_MMAP](./src/capture/mmap/)        | Captures data from another application via shared memory. Requires patching the application to support this interface.                                      |
