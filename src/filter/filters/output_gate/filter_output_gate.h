/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_OUTPUT_GATE_FILTER_OUTPUT_GATE_H
#define VCS_FILTER_FILTERS_OUTPUT_GATE_FILTER_OUTPUT_GATE_H

#include "filter/abstract_filter.h"
#include "filter/filters/output_gate/gui/filtergui_output_gate.h"

class filter_output_gate_c : public abstract_filter_c
{
public:
    enum {
        PARAM_WIDTH,
        PARAM_HEIGHT,
        PARAM_IS_WIDTH_ENABLED,
        PARAM_IS_HEIGHT_ENABLED
    };

    filter_output_gate_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_WIDTH, 640},
            {PARAM_HEIGHT, 480},
            {PARAM_IS_WIDTH_ENABLED, true},
            {PARAM_IS_HEIGHT_ENABLED, true}
        }, initialParamValues)
    {
        this->gui = new filtergui_output_gate_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_output_gate_c)

    std::string uuid(void) const override { return "be8443e2-4355-40fd-aded-63cebcbfb8ce"; }
    std::string name(void) const override { return "Output condition"; }
    filter_category_e category(void) const override { return filter_category_e::output_condition; }
    void apply(image_s *const image) override;

private:
};

#endif
