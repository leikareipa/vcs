/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_UNSHARP_MASK_FILTER_UNSHARP_MASK_H
#define VCS_FILTER_FILTERS_UNSHARP_MASK_FILTER_UNSHARP_MASK_H

#include <opencv2/imgproc/imgproc.hpp>
#include "filter/abstract_filter.h"

class filter_unsharp_mask_c : public abstract_filter_c
{
public:
    enum { PARAM_STRENGTH,
           PARAM_RADIUS };

    filter_unsharp_mask_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({{PARAM_STRENGTH, 5},
                           {PARAM_RADIUS, 1}},
                          initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            auto *strength = filtergui::spinner(filter, filter_unsharp_mask_c::PARAM_STRENGTH);
            strength->minValue = 0;
            strength->maxValue = 255;
            gui->fields.push_back({"Strength", {strength}});

            auto *radius = filtergui::spinner(filter, filter_unsharp_mask_c::PARAM_RADIUS);
            radius->minValue = 1;
            radius->maxValue = 255;
            gui->fields.push_back({"Radius", {radius}});
        });
    }

    CLONABLE_FILTER_TYPE(filter_unsharp_mask_c)

    std::string uuid(void) const override { return "03847778-bb9c-4e8c-96d5-0c10335c4f34"; }
    std::string name(void) const override { return "Unsharp mask"; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }
    void apply(image_s *const image)
    {
        static uint8_t *const tmpBuf = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

        const double str = this->parameter(PARAM_STRENGTH);
        const double rad = this->parameter(PARAM_RADIUS);
        cv::Mat tmp = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, tmpBuf);
        cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
        
        cv::GaussianBlur(output, tmp, cv::Size(0, 0), rad);
        cv::addWeighted(output, 1 + str, tmp, -str, 0, output);

        return;
    }


private:
};

#endif
