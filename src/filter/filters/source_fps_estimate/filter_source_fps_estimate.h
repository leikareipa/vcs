/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_FRAME_RATE_FILTER_FRAME_RATE_H
#define VCS_FILTER_FILTERS_FRAME_RATE_FILTER_FRAME_RATE_H

#include <chrono>
#include "filter/abstract_filter.h"
#include "filter/filters/source_fps_estimate/gui/filtergui_source_fps_estimate.h"

class filter_frame_rate_c : public abstract_filter_c
{
public:
    enum {
        PARAM_THRESHOLD,
        PARAM_CORNER,
        PARAM_TEXT_COLOR,
        PARAM_BG_ALPHA,
        PARAM_IS_SINGLE_ROW_ENABLED,
        PARAM_SINGLE_ROW_NUMBER
    };

    enum {
        TOP_LEFT,
        TOP_RIGHT,
        BOTTOM_RIGHT,
        BOTTOM_LEFT
    };

    enum {
        TEXT_GREEN,
        TEXT_PURPLE,
        TEXT_RED,
        TEXT_YELLOW,
        TEXT_WHITE
    };

    filter_frame_rate_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_THRESHOLD, 20},
            {PARAM_CORNER, TOP_LEFT},
            {PARAM_TEXT_COLOR, TEXT_YELLOW},
            {PARAM_BG_ALPHA, 255},
            {PARAM_IS_SINGLE_ROW_ENABLED, false},
            {PARAM_SINGLE_ROW_NUMBER, 0}
        }, initialParamValues)
    {
        this->gui = new filtergui_source_fps_estimate_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_frame_rate_c)

    std::string uuid(void) const override { return "badb0129-f48c-4253-a66f-b0ec94e225a0"; }
    std::string name(void) const override { return "Source FPS estimate"; }
    filter_category_e category(void) const override { return filter_category_e::meta; }
    void apply(image_s *const image) override;

private:
};

#endif
