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
#include "filter/filters/render_text/filter_render_text.h"
#include "filter/filters/render_text/font_fraps.h"

class filter_frame_rate_c : public abstract_filter_c
{
public:
    enum {
        PARAM_THRESHOLD,
        PARAM_CORNER,
        PARAM_TEXT_COLOR,
        PARAM_BG_ALPHA,
        PARAM_IS_SINGLE_ROW_ENABLED,
        PARAM_SINGLE_ROW_NUMBER,
        PARAM_SHOW_AVERAGE,
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
            {PARAM_SINGLE_ROW_NUMBER, 0},
            {PARAM_SHOW_AVERAGE, 0}
        }, initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            auto *threshold = filtergui::spinner(filter, filter_frame_rate_c::PARAM_THRESHOLD);
            threshold->minValue = 0;
            threshold->maxValue = 255;
            gui->fields.push_back({"Threshold", {threshold}});

            auto *singleRowNumber = filtergui::spinner(filter, filter_frame_rate_c::PARAM_SINGLE_ROW_NUMBER);
            singleRowNumber->isEnabled = filter->parameter(filter_frame_rate_c::PARAM_IS_SINGLE_ROW_ENABLED);
            singleRowNumber->minValue = 0;
            singleRowNumber->maxValue = MAX_CAPTURE_HEIGHT;
            auto *toggleRow = filtergui::checkbox(filter, filter_frame_rate_c::PARAM_IS_SINGLE_ROW_ENABLED);
            toggleRow->on_change = [=](const bool value){
                filter->set_parameter(filter_frame_rate_c::PARAM_IS_SINGLE_ROW_ENABLED, value);
                singleRowNumber->set_enabled(value);
            };
            gui->fields.push_back({"One row", {toggleRow, singleRowNumber}});

            auto *corner = filtergui::combo_box(filter, filter_frame_rate_c::PARAM_CORNER);
            corner->items = {"Top left", "Top right", "Bottom right", "Bottom left"};
            gui->fields.push_back({"Position", {corner}});

            auto *textColor = filtergui::combo_box(filter, filter_frame_rate_c::PARAM_TEXT_COLOR);
            textColor->items = {"Green", "Purple", "Red", "Yellow", "White"};
            gui->fields.push_back({"Fg. color", {textColor}});

            auto *bgAlpha = filtergui::spinner(filter, filter_frame_rate_c::PARAM_BG_ALPHA);
            bgAlpha->minValue = 0;
            bgAlpha->maxValue = 255;
            gui->fields.push_back({"Bg. alpha", {bgAlpha}});
        });
    }

    CLONABLE_FILTER_TYPE(filter_frame_rate_c)

    std::string uuid(void) const override { return "badb0129-f48c-4253-a66f-b0ec94e225a0"; }
    std::string name(void) const override { return "Source FPS"; }
    filter_category_e category(void) const override { return filter_category_e::meta; }
    void apply(image_s *const image)
    {
        static const auto FONT = font_fraps_c();
        static const unsigned FONT_SIZE = 2;

        static uint8_t *const prevPixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();
        static auto timeOfLastUpdate = std::chrono::system_clock::now();
        static unsigned numUniqueFrames = 0;
        static unsigned estimatedFPS = 0;

        const unsigned threshold = this->parameter(PARAM_THRESHOLD);
        const unsigned cornerId = this->parameter(PARAM_CORNER);
        const unsigned fgColorId = this->parameter(PARAM_TEXT_COLOR);
        const uint8_t bgAlpha = this->parameter(PARAM_BG_ALPHA);
        const bool isSingleRow = this->parameter(PARAM_IS_SINGLE_ROW_ENABLED);
        const unsigned rowNumber = this->parameter(PARAM_SINGLE_ROW_NUMBER);

        // Find out whether any pixel in the current frame differs from the previous frame
        // by more than the threshold.
        {
            const unsigned imageBitsPerPixel = (image->bitsPerPixel / 8);
            const unsigned imageByteSize = (image->resolution.w * image->resolution.h * imageBitsPerPixel);
            const unsigned start = (
                isSingleRow
                    ? (rowNumber * image->resolution.w * imageBitsPerPixel)
                    : 0
            );
            const unsigned end = std::min(imageByteSize,
                isSingleRow
                    ? unsigned(start + (image->resolution.w * imageBitsPerPixel))
                    : imageByteSize
            );

            for (unsigned i = start; i < end; i += imageBitsPerPixel)
            {
                if (
                    (std::abs(int(image->pixels[i + 0]) - int(prevPixels[i + 0])) >= threshold) ||
                    (std::abs(int(image->pixels[i + 1]) - int(prevPixels[i + 1])) >= threshold) ||
                    (std::abs(int(image->pixels[i + 2]) - int(prevPixels[i + 2])) >= threshold)
                ){
                    numUniqueFrames++;
                    memcpy(prevPixels, image->pixels, imageByteSize);
                    break;
                }
            }
        }

        // Update the FPS reading.
        {
            const auto timeNow = std::chrono::system_clock::now();
            const double timeElapsed = (std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - timeOfLastUpdate).count() / 500.0);

            if (timeElapsed >= 1)
            {
                estimatedFPS = std::round(2 * (numUniqueFrames / timeElapsed));
                numUniqueFrames = 0;
                timeOfLastUpdate = timeNow;
            }
        }

        // Draw the FPS counter into the image.
        {
            const unsigned signalRefreshRate = refresh_rate_s::from_capture_device_properties().value<unsigned>();
            const std::string outputString = std::to_string(std::min(estimatedFPS, signalRefreshRate));

            const std::pair<unsigned, unsigned> screenCoords = ([cornerId, &outputString, image]()->std::pair<unsigned, unsigned>
            {
                const unsigned textWidth = (FONT_SIZE * FONT.width_of(outputString));
                const unsigned textHeight = (FONT_SIZE * FONT.height_of(outputString));

                switch (cornerId)
                {
                    default:
                    case TOP_LEFT: return {0, 0};
                    case TOP_RIGHT: return {(image->resolution.w - textWidth), 0};
                    case BOTTOM_RIGHT: return {(image->resolution.w - textWidth), (image->resolution.h - textHeight)};
                    case BOTTOM_LEFT: return {0, (image->resolution.h - textHeight)};
                }
            })();

            const auto fgColor = ([fgColorId]()->std::vector<uint8_t>
            {
                switch (fgColorId)
                {
                    default:
                    case TEXT_GREEN: return {0, 255, 0};
                    case TEXT_PURPLE: return {255, 0, 255};
                    case TEXT_RED: return {0, 0, 255};
                    case TEXT_YELLOW: return {0, 255, 255};
                    case TEXT_WHITE: return {255, 255, 255};
                }
            })();

            FONT.render(outputString, image, screenCoords.first, screenCoords.second, FONT_SIZE, fgColor, {0, 0, 0, bgAlpha});
        }

        return;
    }

private:
};

#endif
