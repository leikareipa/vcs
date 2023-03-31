/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_OUTPUT_SCALER_FILTER_OUTPUT_SCALER_H
#define VCS_FILTER_FILTERS_OUTPUT_SCALER_FILTER_OUTPUT_SCALER_H

#include <array>
#include "filter/abstract_filter.h"
#include "filter/filters/output_scaler/gui/filtergui_output_scaler.h"
#include "display/display.h"

struct captured_frame_s;

class filter_output_scaler_c : public abstract_filter_c
{
public:
    enum {
        PARAM_WIDTH,
        PARAM_HEIGHT,
        PARAM_SCALER,
        PARAM_PADDING_TOP,
        PARAM_PADDING_BOTTOM,
        PARAM_PADDING_LEFT,
        PARAM_PADDING_RIGHT,
    };

    enum {
        SCALER_NEAREST,
        SCALER_LINEAR,
        SCALER_AREA,
        SCALER_CUBIC,
        SCALER_LANCZOS,
    };

    filter_output_scaler_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_WIDTH, 640},
            {PARAM_HEIGHT, 480},
            {PARAM_SCALER, SCALER_NEAREST},
            {PARAM_PADDING_TOP, 0},
            {PARAM_PADDING_BOTTOM, 0},
            {PARAM_PADDING_LEFT, 0},
            {PARAM_PADDING_RIGHT, 0},
        },initialParamValues)
    {
        this->guiDescription = new filtergui_output_scaler_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_output_scaler_c)

    std::string uuid(void) const override { return "5365fc0e-78a8-4262-a9eb-e5ef1941a9ed"; }
    std::string name(void) const override { return "Output scaler"; }
    filter_category_e category(void) const override { return filter_category_e::output_scaler; }
    void apply(image_s *const image) override;

    // Returns the resolution of the output image, including any padding.
    resolution_s output_resolution(void) const;

    // Scaler functions; for scaling a source image's data into a destination image.
    static void nearest(const image_s &srcImage, image_s *const dstImage, const std::array<unsigned, 4> padding = {0, 0, 0, 0});
    static void linear(const image_s &srcImage, image_s *const dstImage, const std::array<unsigned, 4> padding = {0, 0, 0, 0});
    static void area(const image_s &srcImage, image_s *const dstImage, const std::array<unsigned, 4> padding = {0, 0, 0, 0});
    static void cubic(const image_s &srcImage, image_s *const dstImage, const std::array<unsigned, 4> padding = {0, 0, 0, 0});
    static void lanczos(const image_s &srcImage, image_s *const dstImage, const std::array<unsigned, 4> padding = {0, 0, 0, 0});
private:
};

#endif
