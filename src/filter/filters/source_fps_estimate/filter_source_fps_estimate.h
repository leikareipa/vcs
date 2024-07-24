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
        _DEPRECATED_PARAM_TEXT_COLOR,
        _DEPRECATED_PARAM_BG_ALPHA,
        PARAM_IS_SINGLE_ROW_ENABLED,
        PARAM_SINGLE_ROW_NUMBER,
        PARAM_IS_AVERAGE_ENABLED,
        PARAM_VISUALIZE_RANGE,
        PARAM_AVERAGE_WINDOW,
    };

    enum {
        TOP_LEFT,
        TOP_RIGHT,
        BOTTOM_RIGHT,
        BOTTOM_LEFT
    };

    filter_frame_rate_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_THRESHOLD, 20},
            {PARAM_CORNER, TOP_LEFT},
            {_DEPRECATED_PARAM_TEXT_COLOR, 0},
            {_DEPRECATED_PARAM_BG_ALPHA, 0},
            {PARAM_IS_SINGLE_ROW_ENABLED, false},
            {PARAM_SINGLE_ROW_NUMBER, 0},
            {PARAM_IS_AVERAGE_ENABLED, false},
            {PARAM_VISUALIZE_RANGE, 0},
            {PARAM_AVERAGE_WINDOW, 9},
        }, initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            auto *singleRowNumber = filtergui::spinner(filter, filter_frame_rate_c::PARAM_SINGLE_ROW_NUMBER);
            singleRowNumber->isEnabled = filter->parameter(filter_frame_rate_c::PARAM_IS_SINGLE_ROW_ENABLED);
            singleRowNumber->minValue = 0;
            singleRowNumber->maxValue = MAX_CAPTURE_HEIGHT;
            auto *toggleRow = filtergui::checkbox(filter, filter_frame_rate_c::PARAM_IS_SINGLE_ROW_ENABLED);
            toggleRow->on_change = [=](const bool value){
                filter->set_parameter(filter_frame_rate_c::PARAM_IS_SINGLE_ROW_ENABLED, value);
                singleRowNumber->set_enabled(value);
            };
            gui->fields.push_back({"Row", {toggleRow, singleRowNumber}});

            auto *averageOver = filtergui::spinner(filter, filter_frame_rate_c::PARAM_AVERAGE_WINDOW);
            averageOver->suffix = " s";
            averageOver->isEnabled = filter->parameter(filter_frame_rate_c::PARAM_AVERAGE_WINDOW);
            averageOver->minValue = 1;
            averageOver->maxValue = 99999;
            auto *toggleAverage = filtergui::checkbox(filter, filter_frame_rate_c::PARAM_IS_AVERAGE_ENABLED);
            toggleAverage->on_change = [=](const bool value){
                filter->set_parameter(filter_frame_rate_c::PARAM_IS_AVERAGE_ENABLED, value);
                averageOver->set_enabled(value);
            };
            gui->fields.push_back({"Average", {toggleAverage, averageOver}});

            auto *threshold = filtergui::spinner(filter, filter_frame_rate_c::PARAM_THRESHOLD);
            threshold->minValue = 0;
            threshold->maxValue = 255;
            gui->fields.push_back({"Threshold", {threshold}});

            auto *corner = filtergui::combo_box(filter, filter_frame_rate_c::PARAM_CORNER);
            corner->items = {"Top left", "Top right", "Bottom right", "Bottom left"};
            gui->fields.push_back({"Position", {corner}});

            auto *visualize = filtergui::combo_box(filter, filter_frame_rate_c::PARAM_VISUALIZE_RANGE);
            visualize->items = {"None", "Range"};
            gui->fields.push_back({"Visualize", {visualize}});
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
        const bool isSingleRow = this->parameter(PARAM_IS_SINGLE_ROW_ENABLED);
        const unsigned rowNumber = this->parameter(PARAM_SINGLE_ROW_NUMBER);
        const int isAverage = this->parameter(PARAM_IS_AVERAGE_ENABLED);
        const int averageTimeout = this->parameter(PARAM_AVERAGE_WINDOW);

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

        // Visualize parameters.
        if (this->parameter(PARAM_VISUALIZE_RANGE))
        {
            const unsigned patternDensity = 9;

            if (isSingleRow)
            {
                for (unsigned x = 0; x < image->resolution.w; x++)
                {
                    if (((x / patternDensity) % 2) == 0)
                    {
                        int idx = ((x + rowNumber * image->resolution.w) * 4);
                        image->pixels[idx + 0] = ~image->pixels[idx + 0];
                        image->pixels[idx + 1] = ~image->pixels[idx + 1];
                        image->pixels[idx + 2] = ~image->pixels[idx + 2];
                    }
                }
            }
            else
            {
                const unsigned startRow = 0;
                const unsigned endRow = (std::max(1u, resolution_s::from_capture_device_properties().h) - 1);

                // Shade the area under the scan range.
                for (unsigned y = startRow; y < endRow; y++)
                {
                    for (unsigned x = 0; x < image->resolution.w; x++)
                    {
                        const unsigned idx = ((x + y * image->resolution.w) * 4);

                        image->pixels[idx + 1] *= 0.5;
                        image->pixels[idx + 2] *= 0.5;

                        // Create a dot pattern.
                        if (((y % patternDensity) == 0) &&
                            ((x + y) % (patternDensity * 2)) == 0)
                        {
                            image->pixels[idx + 0] = ~image->pixels[idx + 0];
                            image->pixels[idx + 1] = ~image->pixels[idx + 1];
                            image->pixels[idx + 2] = ~image->pixels[idx + 2];
                        }
                    }
                }

                // Indicate with a line where the scan range starts and ends.
                for (unsigned x = 0; x < image->resolution.w; x++)
                {
                    if (((x / patternDensity) % 2) == 0)
                    {
                        int idx = ((x + startRow * image->resolution.w) * 4);
                        image->pixels[idx + 0] = ~image->pixels[idx + 0];
                        image->pixels[idx + 1] = ~image->pixels[idx + 1];
                        image->pixels[idx + 2] = ~image->pixels[idx + 2];

                        idx = ((x + endRow * image->resolution.w) * 4);
                        image->pixels[idx + 0] = ~image->pixels[idx + 0];
                        image->pixels[idx + 1] = ~image->pixels[idx + 1];
                        image->pixels[idx + 2] = ~image->pixels[idx + 2];
                    }
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

        // Display the FPS counter.
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

            FONT.render(
                outputString,
                image,
                screenCoords.first,
                screenCoords.second,
                FONT_SIZE,
                {0, 255, 255},
                {0, 0, 0, 255}
            );
        }

        // Display the average FPS counter.
        if (isAverage)
        {
            const unsigned signalRefreshRate = refresh_rate_s::from_capture_device_properties().value<unsigned>();
            this->fpsCache.push_back(std::min(estimatedFPS, signalRefreshRate));

            static unsigned curAverage = 0;
            static auto prevTime = std::chrono::steady_clock::now();

            auto curTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(curTime - prevTime);
            const int timeLeft = (averageTimeout - elapsedTime.count());

            if (timeLeft <= 0) {
                const unsigned sum = std::accumulate(this->fpsCache.begin(), this->fpsCache.end(), 0);
                curAverage = (double(sum) / this->fpsCache.size());
                this->fpsCache.clear();
                prevTime = curTime;
            }

            const std::string outputString = (
                ((cornerId == TOP_LEFT) || (cornerId == BOTTOM_LEFT))
                    ? (std::to_string(curAverage) + ":" + std::to_string(std::max(1, timeLeft)))
                    : (std::to_string(std::max(1, timeLeft)) + ":" + std::to_string(curAverage))
            );

            const std::pair<unsigned, unsigned> screenCoords = ([cornerId, &outputString, image]()->std::pair<unsigned, unsigned>
            {
                const unsigned textWidth = (FONT_SIZE * FONT.width_of(outputString));
                const unsigned textHeight = (FONT_SIZE * FONT.height_of(outputString));

                switch (cornerId)
                {
                    default:
                    case TOP_LEFT: return {0, textHeight};
                    case TOP_RIGHT: return {(image->resolution.w - textWidth), textHeight};
                    case BOTTOM_RIGHT: return {(image->resolution.w - textWidth), (image->resolution.h - textHeight * 2)};
                    case BOTTOM_LEFT: return {0, (image->resolution.h - textHeight * 2)};
                }
            })();

            FONT.render(
                outputString,
                image,
                screenCoords.first,
                screenCoords.second,
                FONT_SIZE,
                {0, 0, 192},
                {0, 0, 0, 255}
            );
        }

        return;
    }

private:
    std::vector<unsigned> fpsCache;
};

#endif
