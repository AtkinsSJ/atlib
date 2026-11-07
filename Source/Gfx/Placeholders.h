/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/RenderItem.h>

class DrawRectPlaceholder {
public:
    void fill(Rect2 bounds, Colour);
    void fill(Rect2 bounds, Colour colour00, Colour colour01, Colour colour10, Colour colour11);
    void fill(Rect2 bounds, Sprite const&, Colour);

    void set_texture_item(Badge<RenderBuffer>, RenderItem_SetTexture&);
    void set_rect_item(Badge<RenderBuffer>, RenderItem_DrawSingleRect&);

private:
    RenderItem_SetTexture* m_set_texture_item { nullptr };
    RenderItem_DrawSingleRect* m_draw_rect_item { nullptr };
};

class DrawNinepatchPlaceholder {
public:
    explicit DrawNinepatchPlaceholder(Span<RenderItem_DrawRects_Item>);
    void fill(Rect2I bounds, Ninepatch const&, Colour);

private:
    Span<RenderItem_DrawRects_Item> m_rects;
};
