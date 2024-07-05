/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/output_scaler/filter_output_scaler.h"
#include "filter/filters/output_scaler/gui/filtergui_output_scaler.h"
#include "scaler/scaler.h"

filtergui_output_scaler_c::filtergui_output_scaler_c(abstract_filter_c *const filter)
{
    auto *scaledWidth = filtergui::spinner(filter, filter_output_scaler_c::PARAM_WIDTH);
    scaledWidth->minValue = MIN_OUTPUT_WIDTH;
    scaledWidth->maxValue = MAX_OUTPUT_WIDTH;
    auto *scaledHeight = filtergui::spinner(filter, filter_output_scaler_c::PARAM_HEIGHT);
    scaledHeight->minValue = MIN_OUTPUT_HEIGHT;
    scaledHeight->maxValue = MAX_OUTPUT_HEIGHT;
    this->fields.push_back({"Size", {scaledWidth, scaledHeight}});

    auto *scalerName = filtergui::combo_box(filter, filter_output_scaler_c::PARAM_SCALER);
    scalerName->items = ks_scaler_names();
    this->fields.push_back({"Sampler", {scalerName}});

    auto *topPadding = filtergui::spinner(filter, filter_output_scaler_c::PARAM_PADDING_TOP);
    topPadding->minValue = 0;
    topPadding->maxValue = MAX_OUTPUT_HEIGHT;
    topPadding->alignment = abstract_gui_widget::horizontal_align::center;
    auto *bottomPadding = filtergui::spinner(filter, filter_output_scaler_c::PARAM_PADDING_BOTTOM);
    bottomPadding->minValue = 0;
    bottomPadding->maxValue = MAX_OUTPUT_HEIGHT;
    bottomPadding->alignment = abstract_gui_widget::horizontal_align::center;
    auto *leftPadding = filtergui::spinner(filter, filter_output_scaler_c::PARAM_PADDING_LEFT);
    leftPadding->minValue = 0;
    leftPadding->maxValue = MAX_OUTPUT_WIDTH;
    leftPadding->alignment = abstract_gui_widget::horizontal_align::center;
    auto *rightPadding = filtergui::spinner(filter, filter_output_scaler_c::PARAM_PADDING_RIGHT);
    rightPadding->minValue = 0;
    rightPadding->maxValue = MAX_OUTPUT_WIDTH;
    rightPadding->alignment = abstract_gui_widget::horizontal_align::center;
    this->fields.push_back({"Padding", {topPadding}});
    this->fields.push_back({"", {leftPadding, rightPadding}});
    this->fields.push_back({"", {bottomPadding}});

    return;
}
