/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 */

#ifndef VCS_FILTER_FILTERS_RENDER_TEXT_FONT_FRAPS_H
#define VCS_FILTER_FILTERS_RENDER_TEXT_FONT_FRAPS_H

#include <unordered_map>
#include "filter/filters/render_text/font.h"

// A Fraps-like numeric font.
class font_fraps_c : public font_c
{
public:
    unsigned cap_height() const override
    {
        return 12;
    }

    unsigned letter_spacing() const override
    {
        return 3;
    }

    unsigned line_spacing() const override
    {
        return (this->letter_spacing() * 2);
    }

    const font_glyph_s& glyph_at(const char key) const override
    {
        // This font supports only uppercase symbols.
        const char upperKey = std::toupper(key);

        return (
            (this->charset.find(upperKey) == this->charset.end())
            ? this->charset.at('?')
            : this->charset.at(upperKey)
        );
    }

private:
    #define X 1
    // Replicates the numeric glyphs from Fraps.
    const std::unordered_map<char, font_glyph_s> charset = {
        {'?', {{{X,X,X,X},
                {0,0,0,X},
                {0,0,0,X},
                {0,X,X,X},
                {0,X,0,0},
                {0,0,0,0},
                {0,X,0,0}}}},

        {'+', {{{0,0,X,X,0,0},
                {0,0,X,X,0,0},
                {X,X,X,X,X,X},
                {X,X,X,X,X,X},
                {0,0,X,X,0,0},
                {0,0,X,X,0,0}}, 3}},

        {' ', {{{0,0,0,0,0,0,0}}}},

        {'.', {{{{X,X},
                 {X,X}}}}},

        {':', {{{X,X},
                {X,X},
                {0,0},
                {0,0},
                {0,0},
                {X,X},
                {X,X}}, 2}},

        {'0', {{{X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X}}}},

        {'1', {{{0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X}}}},

        {'2', {{{X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {X,X,0,0,0,0,0},
                {X,X,0,0,0,0,0},
                {X,X,0,0,0,0,0},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X}}}},

        {'3', {{{X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X}}}},

        {'4', {{{X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X}}}},

        {'5', {{{X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {X,X,0,0,0,0,0},
                {X,X,0,0,0,0,0},
                {X,X,0,0,0,0,0},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X}}}},

        {'6', {{{X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {X,X,0,0,0,0,0},
                {X,X,0,0,0,0,0},
                {X,X,0,0,0,0,0},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X}}}},

        {'7', {{{X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X}}}},

        {'8', {{{X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X}}}},

        {'9', {{{X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,0,0,0,X,X},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {0,0,0,0,0,X,X},
                {X,X,X,X,X,X,X},
                {X,X,X,X,X,X,X}}}},
    };
    #undef X
};

#endif
