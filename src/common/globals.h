/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_COMMON_GLOBALS_H
#define VCS_COMMON_GLOBALS_H

#include <stdint.h>

const char PROGRAM_NAME[] = "VCS";

const struct semantic_version_s
{
    unsigned major = 3;
    unsigned minor = 6;
    unsigned patch = 0;
} VCS_VERSION;

// The minimum and maximum resolution we can output frames in.
const unsigned MIN_OUTPUT_WIDTH = 160;
const unsigned MIN_OUTPUT_HEIGHT = 100;
const unsigned MAX_OUTPUT_WIDTH = 4096;
const unsigned MAX_OUTPUT_HEIGHT = 4096;
const unsigned MAX_OUTPUT_BPP = 32;
const uint64_t MAX_NUM_BYTES_IN_OUTPUT_FRAME = (MAX_OUTPUT_WIDTH * MAX_OUTPUT_HEIGHT * uint64_t(MAX_OUTPUT_BPP / 8));

// The minimum and maximum resolution we can accept frames from the capture
// device in.
const unsigned MIN_CAPTURE_WIDTH = 1;
const unsigned MIN_CAPTURE_HEIGHT = 1;
const unsigned MAX_CAPTURE_WIDTH = 4096;
const unsigned MAX_CAPTURE_HEIGHT = 4096;
const unsigned MAX_CAPTURE_BPP = 32;
const uint64_t MAX_NUM_BYTES_IN_CAPTURED_FRAME = (MAX_CAPTURE_WIDTH * MAX_CAPTURE_HEIGHT * uint64_t(MAX_CAPTURE_BPP / 8));

extern unsigned FRAME_SKIP;
extern unsigned INPUT_CHANNEL_IDX;
extern bool PROGRAM_EXIT_REQUESTED;

#endif
