/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/input_gate/filter_input_gate.h"

void filter_input_gate_c::apply(u8 *const pixels, const resolution_s &r)
{
    /// Input gates do not modify pixel data.

    (void)pixels;
    (void)r;

    return;
}
