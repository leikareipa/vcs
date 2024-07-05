/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DENOISE_PIXEL_GATE_FILTER_DENOISE_PIXEL_GATE_H
#define VCS_FILTER_FILTERS_DENOISE_PIXEL_GATE_FILTER_DENOISE_PIXEL_GATE_H

#include "filter/abstract_filter.h"

class filter_denoise_pixel_gate_c : public abstract_filter_c
{
public:
    enum { PARAM_STRENGTH };
                         
    filter_denoise_pixel_gate_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({{PARAM_STRENGTH, 5}}, initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            auto *strength = filtergui::spinner(filter, filter_denoise_pixel_gate_c::PARAM_STRENGTH);
            strength->minValue = 0;
            strength->maxValue = 255;
            gui->fields.push_back({"Strength", {strength}});
        });
    }

    CLONABLE_FILTER_TYPE(filter_denoise_pixel_gate_c)

    std::string uuid(void) const override { return "94adffac-be42-43ac-9839-9cc53a6d615c"; }
    std::string name(void) const override { return "Temporal denoise"; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }
    void apply(image_s *const image)
    {
        const unsigned threshold = this->parameter(PARAM_STRENGTH);
        static uint8_t *const prevPixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();
        static uint8_t *const absoluteDiff = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

        cv::absdiff(
            cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels),
            cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, prevPixels),
            cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, absoluteDiff)
        );

        for (unsigned i = 0; i < image->byte_size(); i += 4)
        {
            if (
                (absoluteDiff[i + 0] > threshold) ||
                (absoluteDiff[i + 1] > threshold) ||
                (absoluteDiff[i + 2] > threshold)
            ){
                prevPixels[i + 0] = image->pixels[i + 0];
                prevPixels[i + 1] = image->pixels[i + 1];
                prevPixels[i + 2] = image->pixels[i + 2];
            }
            else
            {
                image->pixels[i + 0] = prevPixels[i + 0];
                image->pixels[i + 1] = prevPixels[i + 1];
                image->pixels[i + 2] = prevPixels[i + 2];
            }
        }

        return;
    }

private:
};

#endif
