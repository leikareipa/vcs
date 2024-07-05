/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_ROTATE_FILTER_ROTATE_H
#define VCS_FILTER_FILTERS_ROTATE_FILTER_ROTATE_H

#include <opencv2/imgproc/imgproc.hpp>
#include "filter/abstract_filter.h"

class filter_rotate_c : public abstract_filter_c
{
public:
    enum { PARAM_ROT,
           PARAM_SCALE };

    filter_rotate_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({{PARAM_SCALE, 1},
                           {PARAM_ROT, 0}},
                          initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            auto *angle = filtergui::double_spinner(filter, filter_rotate_c::PARAM_ROT);
            angle->minValue = -360;
            angle->maxValue = 360;
            angle->numDecimals = 2;
            gui->fields.push_back({"Angle", {angle}});

            auto *scale = filtergui::double_spinner(filter, filter_rotate_c::PARAM_SCALE);
            scale->minValue = 0.01;
            scale->maxValue = 20;
            scale->numDecimals = 2;
            scale->stepSize = 0.1;
            gui->fields.push_back({"Scale", {scale}});
        });
    }

    CLONABLE_FILTER_TYPE(filter_rotate_c)

    std::string uuid(void) const override { return "140c514d-a4b0-4882-abc6-b4e9e1ff4451"; }
    std::string name(void) const override { return "Rotate"; }
    filter_category_e category(void) const override { return filter_category_e::distort; }
    void apply(image_s *const image)
    {
        static uint8_t *const scratch = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();
        
        const double angle = this->parameter(PARAM_ROT);
        const double scale = this->parameter(PARAM_SCALE);
        cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
        cv::Mat temp = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, scratch);
        cv::Mat transf = cv::getRotationMatrix2D(cv::Point2d((image->resolution.w / 2), (image->resolution.h / 2)), -angle, scale);
        
        cv::warpAffine(output, temp, transf, cv::Size(image->resolution.w, image->resolution.h));
        temp.copyTo(output);

        return;
    }

private:
};

#endif
