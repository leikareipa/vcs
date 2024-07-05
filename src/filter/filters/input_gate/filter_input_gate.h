/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_INPUT_GATE_FILTER_INPUT_GATE_H
#define VCS_FILTER_FILTERS_INPUT_GATE_FILTER_INPUT_GATE_H

#include "filter/abstract_filter.h"
#include "filter/filters/input_gate/gui/filtergui_input_gate.h"

class filter_input_gate_c : public abstract_filter_c
{
public:
    enum {
        PARAM_WIDTH,
        PARAM_HEIGHT,
        PARAM_IS_WIDTH_ENABLED,
        PARAM_IS_HEIGHT_ENABLED,
        PARAM_HZ,
        PARAM_IS_HZ_ENABLED,
    };

    filter_input_gate_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_WIDTH, 640},
            {PARAM_HEIGHT, 480},
            {PARAM_HZ, 60},
            {PARAM_IS_WIDTH_ENABLED, true},
            {PARAM_IS_HEIGHT_ENABLED, true},
            {PARAM_IS_HZ_ENABLED, false}
        }, initialParamValues)
    {
        this->gui = new filtergui_input_gate_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_input_gate_c)

    std::string uuid(void) const override { return "136deb34-ac79-46b1-a09c-d57dcfaa84ad"; }
    std::string name(void) const override { return "Input condition"; }
    filter_category_e category(void) const override { return filter_category_e::input_condition; }
    void apply(image_s *const image) override;

private:
};

#endif
