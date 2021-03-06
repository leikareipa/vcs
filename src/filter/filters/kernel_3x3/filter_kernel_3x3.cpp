/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/kernel_3x3/filter_kernel_3x3.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
    #include <opencv2/core/core.hpp>
#endif

void filter_kernel_3x3_c::apply(u8 *const pixels, const resolution_s &r)
{
    this->assert_input_validity(pixels, r);

    #ifdef USE_OPENCV
        const float v11 = this->parameter(PARAM_11);
        const float v12 = this->parameter(PARAM_12);
        const float v13 = this->parameter(PARAM_13);
        const float v21 = this->parameter(PARAM_21);
        const float v22 = this->parameter(PARAM_22);
        const float v23 = this->parameter(PARAM_23);
        const float v31 = this->parameter(PARAM_31);
        const float v32 = this->parameter(PARAM_32);
        const float v33 = this->parameter(PARAM_33);

        float kernel[] = {v11, v12, v13,
                          v21, v22, v23,
                          v31, v32, v33};

        cv::Mat ker = cv::Mat(3, 3, CV_32F, &kernel);
        cv::Mat output = cv::Mat(r.h, r.w, CV_8UC4, pixels);
        cv::filter2D(output, output, -1, ker);
    #endif

    return;
}
