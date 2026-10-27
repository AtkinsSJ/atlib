/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "StatBar.h"

#include <Gfx/Renderer.h>

namespace UI {

StatBar::~StatBar() = default;

V2I StatBar::calculate_size(Rect2I const& space, bool fill_width) const
{
    // FIXME: Figure out what our height is! For now, 32px
    return { space.width(), 16 };
}

void StatBar::update_and_render(Rect2I const& bounds, RenderBuffer& buffer)
{
    // FIXME: It'd be nice to still use a style for this... How do we do that?
    auto& renderer = the_renderer();
    drawSingleRect(&buffer, bounds, renderer.shaderIds.untextured, Colour { 0.2, 0.2, 0.2, 1.0 });

    float filled_percent = 1.0f;
    if (m_current >= m_max) {
        filled_percent = 1.0f;
    } else {
        filled_percent = static_cast<float>(m_current) / static_cast<float>(m_max);
    }
    auto filled_bounds = bounds;
    filled_bounds.set_width(round_s32(static_cast<float>(bounds.width()) * filled_percent));
    drawSingleRect(&buffer, filled_bounds, renderer.shaderIds.untextured, Colour { 1.0, 0.2, 0.2, 1.0 });
}

}
