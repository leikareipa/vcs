/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_FLIP_FILTER_FLIP_H
#define VCS_FILTER_FILTERS_FLIP_FILTER_FLIP_H

#include <opencv2/imgproc/imgproc.hpp>
#include "filter/abstract_filter.h"

class filter_flip_c : public abstract_filter_c
{
public:
    enum { PARAM_AXIS };

    filter_flip_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_AXIS, 0}
        }, initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            auto *axis = filtergui::combo_box(filter, filter_flip_c::PARAM_AXIS);
            axis->items = {"Vertical", "Horizontal", "Both"};
            gui->fields.push_back({"Axis", {axis}});
        });
    }

    CLONABLE_FILTER_TYPE(filter_flip_c)

    std::string uuid(void) const override { return "80a3ac29-fcec-4ae0-ad9e-bbd8667cc680"; }
    std::string name(void) const override { return "Flip"; }
    filter_category_e category(void) const override { return filter_category_e::distort; }
    void apply(image_s *const image)
    {
        static uint8_t *const scratch = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

        int axis = this->parameter(PARAM_AXIS);

        // 0 = vertical, 1 = horizontal, -1 = both.
        axis = ((axis == 2)? -1 : axis);

        cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
        cv::Mat temp = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, scratch);

        cv::flip(output, temp, axis);
        temp.copyTo(output);
    }

private:
};

#endif
