/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <Assets/AssetRef.h>
#include <UI/UITheme.h>
#include <UI/Widget.h>

namespace UI {

class StatBarStyle final : public Asset {
    ASSET_SUBCLASS_METHODS(StatBarStyle);

public:
    StatBarStyle(DrawableStyle track, DrawableStyle bar, Alignment bar_alignment, s32 height);
    virtual ~StatBarStyle() override;

    virtual void unload(AssetMetadata& metadata) override { }

    DrawableStyle track;
    DrawableStyle bar;
    Alignment bar_alignment;
    s32 height;
};

class StatBar : public Widget {
public:
    explicit StatBar(String style_name = "default"_s);
    virtual ~StatBar() override;

    void set_current(u32 current) { m_current = current; }
    void set_max(u32 max) { m_max = max; }

    virtual V2I calculate_size(Rect2I const& space, bool fill_width) const override;
    virtual void update_and_render(Rect2I const& bounds, RenderBuffer& buffer) override;

private:
    TypedAssetRef<StatBarStyle> m_style;
    u32 m_current { 1 };
    u32 m_max { 1 };
};

}
