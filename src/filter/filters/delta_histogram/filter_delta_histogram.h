/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DELTA_HISTGRAM_FILTER_DELTA_HISTOGRAM_H
#define VCS_FILTER_FILTERS_DELTA_HISTGRAM_FILTER_DELTA_HISTOGRAM_H

#include "filter/filter.h"
#include "filter/filters/delta_histogram/gui/filtergui_delta_histogram.h"

class filter_delta_histogram_c : public filter_c
{
public:
    filter_delta_histogram_c(FILTER_CTOR_FUNCTION_PARAMS) :
        filter_c({}, initialParameterValues)
    {
        this->guiDescription = new filtergui_delta_histogram_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_delta_histogram_c)

    void apply(FILTER_APPLY_FUNCTION_PARAMS) override;

    std::string uuid(void) const override { return "fc85a109-c57a-4317-994f-786652231773"; }
    std::string name(void) const override { return "Delta histogram"; }
    filter_category_e category(void) const override { return filter_category_e::meta; }

private:
};

#endif
