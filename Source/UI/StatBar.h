/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <UI/Widget.h>

namespace UI {

class StatBar : public Widget {
public:
    virtual ~StatBar() override;

    void set_current(u32 current) { m_current = current; }
    void set_max(u32 max) { m_max = max; }

    virtual V2I calculate_size(Rect2I const& space, bool fill_width) const override;
    virtual void update_and_render(Rect2I const& bounds, RenderBuffer& buffer) override;

private:
    u32 m_current { 1 };
    u32 m_max { 1 };
};

}
