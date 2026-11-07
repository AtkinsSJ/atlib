/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Placeholders.h"

#include <Gfx/Ninepatch.h>
#include <Gfx/Sprite.h>

void DrawRectPlaceholder::fill(Rect2 bounds, Colour colour)
{
    fill(bounds, colour, colour, colour, colour);
}

void DrawRectPlaceholder::fill(Rect2 bounds, Colour colour00, Colour colour01, Colour colour10, Colour colour11)
{
    ASSERT(m_draw_rect_item != nullptr);

    m_draw_rect_item->bounds = bounds;
    m_draw_rect_item->color00 = colour00;
    m_draw_rect_item->color01 = colour01;
    m_draw_rect_item->color10 = colour10;
    m_draw_rect_item->color11 = colour11;
    m_draw_rect_item->uv = { 0.0f, 0.0f, 1.0f, 1.0f };
}

void DrawRectPlaceholder::fill(Rect2 bounds, Sprite const& sprite, Colour colour)
{
    ASSERT(m_draw_rect_item != nullptr);
    ASSERT(m_set_texture_item != nullptr);

    m_set_texture_item->texture = sprite.texture;

    m_draw_rect_item->bounds = bounds;
    m_draw_rect_item->color00 = colour;
    m_draw_rect_item->color01 = colour;
    m_draw_rect_item->color10 = colour;
    m_draw_rect_item->color11 = colour;
    m_draw_rect_item->uv = sprite.uv;
}

void DrawRectPlaceholder::set_texture_item(Badge<RenderBuffer>, RenderItem_SetTexture& set_texture)
{
    m_set_texture_item = &set_texture;
}

void DrawRectPlaceholder::set_rect_item(Badge<RenderBuffer>, RenderItem_DrawSingleRect& draw_rect)
{
    m_draw_rect_item = &draw_rect;
}

DrawNinepatchPlaceholder::DrawNinepatchPlaceholder(Span<RenderItem_DrawRects_Item> rects)
    : m_rects(rects)
{
}

void DrawNinepatchPlaceholder::fill(Rect2I bounds, Ninepatch const& ninepatch, Colour colour)
{
    // @Copypasta drawNinepatch()
    // NB: See comments in the Ninepatch struct for how we could avoid doing the  UV calculations repeatedly,
    // and why we haven't done so.

    float x0 = static_cast<float>(bounds.x());
    float x1 = static_cast<float>(bounds.x() + ninepatch.pu1 - ninepatch.pu0);
    float x2 = static_cast<float>(bounds.x() + bounds.width() - (ninepatch.pu3 - ninepatch.pu2));
    float x3 = static_cast<float>(bounds.x() + bounds.width());

    float y0 = static_cast<float>(bounds.y());
    float y1 = static_cast<float>(bounds.y() + ninepatch.pv1 - ninepatch.pv0);
    float y2 = static_cast<float>(bounds.y() + bounds.height() - (ninepatch.pv3 - ninepatch.pv2));
    float y3 = static_cast<float>(bounds.y() + bounds.height());

    // top left
    m_rects[0].bounds = Rect2::create_min_max(x0, y0, x1, y1);
    m_rects[0].color = colour;
    m_rects[0].uv = Rect2::create_min_max(ninepatch.u0, ninepatch.v0, ninepatch.u1, ninepatch.v1);

    // top
    m_rects[1].bounds = Rect2::create_min_max(x1, y0, x2, y1);
    m_rects[1].color = colour;
    m_rects[1].uv = Rect2::create_min_max(ninepatch.u1, ninepatch.v0, ninepatch.u2, ninepatch.v1);

    // top-right
    m_rects[2].bounds = Rect2::create_min_max(x2, y0, x3, y1);
    m_rects[2].color = colour;
    m_rects[2].uv = Rect2::create_min_max(ninepatch.u2, ninepatch.v0, ninepatch.u3, ninepatch.v1);

    // middle left
    m_rects[3].bounds = Rect2::create_min_max(x0, y1, x1, y2);
    m_rects[3].color = colour;
    m_rects[3].uv = Rect2::create_min_max(ninepatch.u0, ninepatch.v1, ninepatch.u1, ninepatch.v2);

    // middle
    m_rects[4].bounds = Rect2::create_min_max(x1, y1, x2, y2);
    m_rects[4].color = colour;
    m_rects[4].uv = Rect2::create_min_max(ninepatch.u1, ninepatch.v1, ninepatch.u2, ninepatch.v2);

    // middle-right
    m_rects[5].bounds = Rect2::create_min_max(x2, y1, x3, y2);
    m_rects[5].color = colour;
    m_rects[5].uv = Rect2::create_min_max(ninepatch.u2, ninepatch.v1, ninepatch.u3, ninepatch.v2);

    // bottom left
    m_rects[6].bounds = Rect2::create_min_max(x0, y2, x1, y3);
    m_rects[6].color = colour;
    m_rects[6].uv = Rect2::create_min_max(ninepatch.u0, ninepatch.v2, ninepatch.u1, ninepatch.v3);

    // bottom
    m_rects[7].bounds = Rect2::create_min_max(x1, y2, x2, y3);
    m_rects[7].color = colour;
    m_rects[7].uv = Rect2::create_min_max(ninepatch.u1, ninepatch.v2, ninepatch.u2, ninepatch.v3);

    // bottom-right
    m_rects[8].bounds = Rect2::create_min_max(x2, y2, x3, y3);
    m_rects[8].color = colour;
    m_rects[8].uv = Rect2::create_min_max(ninepatch.u2, ninepatch.v2, ninepatch.u3, ninepatch.v3);
}
