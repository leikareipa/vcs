/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_SHARPEN_FILTER_SHARPEN_H
#define VCS_FILTER_FILTERS_SHARPEN_FILTER_SHARPEN_H

#include <opencv2/imgproc/imgproc.hpp>
#include "filter/abstract_filter.h"

class filter_sharpen_c : public abstract_filter_c
{
public:
    filter_sharpen_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({}, initialParamValues)
    {
        this->gui = new abstract_gui_s();
    }

    CLONABLE_FILTER_TYPE(filter_sharpen_c)

    std::string uuid(void) const override { return "1c25bbb1-dbf4-4a03-93a1-adf24b311070"; }
    std::string name(void) const override { return "Sharpen"; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }
    void apply(image_s *const image)
    {
        static float kernel[] = {
             0, -1,  0,
            -1,  5, -1,
             0, -1,  0
        };

        cv::Mat ker = cv::Mat(3, 3, CV_32F, &kernel);
        cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
        cv::filter2D(output, output, -1, ker);

        return;
    }

private:
};

#endif
