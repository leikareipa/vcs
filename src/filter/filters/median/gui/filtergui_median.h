/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_MEDIAN_GUI_FILTERGUI_MEDIAN_H
#define VCS_FILTER_FILTERS_MEDIAN_GUI_FILTERGUI_MEDIAN_H

#include "common/abstract_gui.h"

class filtergui_median_c : public abstract_gui_s
{
public:
    filtergui_median_c(abstract_filter_c *const filter);
};

#endif
