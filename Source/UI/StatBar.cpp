/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "StatBar.h"

#include <Gfx/Renderer.h>
#include <UI/Drawable.h>

namespace UI {

StatBarStyle::StatBarStyle(DrawableStyle track, DrawableStyle bar, Alignment bar_alignment, s32 height)
    : track(move(track))
    , bar(move(bar))
    , bar_alignment(bar_alignment)
    , height(height)
{
}

StatBarStyle::~StatBarStyle() = default;

StatBar::StatBar(String style_name)
    : m_style(style_name)
{
}

StatBar::~StatBar() = default;

V2I StatBar::calculate_size(Rect2I const& space, bool fill_width) const
{
    // FIXME: Prefer a narrower width if we're a repeating sprite?
    auto& style = m_style.get();
    return { space.width(), style.height };
}

void StatBar::update_and_render(Rect2I const& bounds, RenderBuffer& buffer)
{
    auto& style = m_style.get();

    Drawable(&style.track).draw(&buffer, bounds);

    float filled_percent = 1.0f;
    if (m_current >= m_max) {
        filled_percent = 1.0f;
    } else {
        filled_percent = static_cast<float>(m_current) / static_cast<float>(m_max);
    }

    auto filled_bounds = bounds.create_aligned_within({ round_s32(static_cast<float>(bounds.width()) * filled_percent), bounds.height() }, style.bar_alignment);
    Drawable(&style.bar).draw(&buffer, filled_bounds);
}

}
