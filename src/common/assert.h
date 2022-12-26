/*
 * 2018, 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_COMMON_ASSERT_H
#define VCS_COMMON_ASSERT_H

#include "display/display.h"
#include "common/log/log.h"

#ifdef NDEBUG
    #error "NDEBUG disables assertions. Assertions are required by design."
#endif
#include <stdexcept>
#include <cassert>

#define k_assert(condition, error_string) \
    if (!(condition))\
    {\
        klog_log_assert("Assertion failure in %s:%d: \"%s\"", __FILE__, __LINE__, error_string);\
        kd_show_headless_assert_error_message(error_string, __FILE__, __LINE__);\
        throw std::runtime_error(error_string);\
    }

// Assertions in e.g. performance-critical code.
#ifndef RELEASE_BUILD
    #define k_assert_optional k_assert
#else
    #define k_assert_optional(...)
#endif

#endif