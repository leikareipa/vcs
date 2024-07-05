/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_SHARPEN_FILTER_KERNEL_3X3_H
#define VCS_FILTER_FILTERS_SHARPEN_FILTER_KERNEL_3X3_H

#include "filter/abstract_filter.h"

class filter_kernel_3x3_c : public abstract_filter_c
{
public:
    enum { PARAM_11,
           PARAM_12,
           PARAM_13,
           PARAM_21,
           PARAM_22,
           PARAM_23,
           PARAM_31,
           PARAM_32,
           PARAM_33,};

    filter_kernel_3x3_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_11, 0},
            {PARAM_12, 0},
            {PARAM_13, 0},
            {PARAM_21, 0},
            {PARAM_22, 1},
            {PARAM_23, 0},
            {PARAM_31, 0},
            {PARAM_32, 0},
            {PARAM_33, 0}
        }, initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            for (unsigned row = 0; row < 3; row++)
            {
                abstract_gui_widget::double_spinner *cols[3];

                for (unsigned col = 0; col < 3; col++)
                {
                    const unsigned paramId = ((row * 3) + col);

                    auto *const spinbox = cols[col] = filtergui::double_spinner(filter, paramId);
                    spinbox->alignment = abstract_gui_widget::horizontal_align::center;
                    spinbox->maxValue = 99;
                    spinbox->minValue = -99;
                    spinbox->numDecimals = 3;
                }

                gui->fields.push_back({"", {cols[0], cols[1], cols[2]}});
            }
        });
    }

    CLONABLE_FILTER_TYPE(filter_kernel_3x3_c)

    std::string uuid(void) const override { return "95027807-978b-4371-9a14-f6166efc64d9"; }
    std::string name(void) const override { return "3-by-3 kernel"; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }

    void apply(image_s *const image)
    {
        const float v11 = this->parameter(PARAM_11);
        const float v12 = this->parameter(PARAM_12);
        const float v13 = this->parameter(PARAM_13);
        const float v21 = this->parameter(PARAM_21);
        const float v22 = this->parameter(PARAM_22);
        const float v23 = this->parameter(PARAM_23);
        const float v31 = this->parameter(PARAM_31);
        const float v32 = this->parameter(PARAM_32);
        const float v33 = this->parameter(PARAM_33);

        float kernel[] = {
            v11, v12, v13,
            v21, v22, v23,
            v31, v32, v33
        };

        cv::Mat ker = cv::Mat(3, 3, CV_32F, &kernel);
        cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
        cv::filter2D(output, output, -1, ker);

        return;
    }

private:
};

#endif
