/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_OUTPUT_GATE_FILTER_OUTPUT_GATE_H
#define VCS_FILTER_FILTERS_OUTPUT_GATE_FILTER_OUTPUT_GATE_H

#include "filter/filter.h"
#include "filter/filters/output_gate/gui/filtergui_output_gate.h"

class filter_output_gate_c : public filter_c
{
public:
    enum { PARAM_WIDTH,
           PARAM_HEIGHT };

    filter_output_gate_c(FILTER_CTOR_FUNCTION_PARAMS) :
        filter_c({{PARAM_WIDTH, 640},
                  {PARAM_HEIGHT, 480}},
                 initialParameterValues)
    {
        this->guiDescription = new filtergui_output_gate_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_output_gate_c)

    std::string uuid(void) const override { return "be8443e2-4355-40fd-aded-63cebcbfb8ce"; }
    std::string name(void) const override { return "Output gate"; }
    filter_category_e category(void) const override { return filter_category_e::gate_output; }

    void apply(FILTER_APPLY_FUNCTION_PARAMS) override;

private:
};

#endif
