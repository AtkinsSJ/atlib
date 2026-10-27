/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/Forward.h>
#include <Util/Rectangle.h>
#include <Util/Vector.h>

namespace UI {

class Widget {
public:
    virtual ~Widget() = default;

    virtual V2I calculate_size(Rect2I const& space, bool fill_width) const = 0;
    virtual void update_and_render(Rect2I const& bounds, RenderBuffer& buffer) = 0;
};

}
