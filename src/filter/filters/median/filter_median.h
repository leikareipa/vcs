/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_MEDIAN_FILTER_MEDIAN_H
#define VCS_FILTER_FILTERS_MEDIAN_FILTER_MEDIAN_H

#include <opencv2/imgproc/imgproc.hpp>
#include "filter/abstract_filter.h"

class filter_median_c : public abstract_filter_c
{
public:
    enum { PARAM_KERNEL_SIZE };

    filter_median_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({{PARAM_KERNEL_SIZE, 3}}, initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            auto *const radius = new abstract_gui_widget::spinner;
            radius->on_change = [=](const int value){filter->set_parameter(filter_median_c::PARAM_KERNEL_SIZE, ((value * 2) + 1));};
            radius->value = (filter->parameter(filter_median_c::PARAM_KERNEL_SIZE) / 2);
            radius->minValue = 0;
            radius->maxValue = 99;
            gui->fields.push_back({"Radius", {radius}});
        });
    }

    CLONABLE_FILTER_TYPE(filter_median_c)

    std::string uuid(void) const override { return "de60017c-afe5-4e5e-99ca-aca5756da0e8"; }
    std::string name(void) const override { return "Median"; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }
    void apply(image_s *const image)
    {
        const double kernelSize = this->parameter(PARAM_KERNEL_SIZE);
        cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
        cv::medianBlur(output, output, kernelSize);

        return;
    }

private:
};

#endif
