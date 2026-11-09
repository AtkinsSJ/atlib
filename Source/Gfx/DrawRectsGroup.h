/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/RenderItem.h>

class DrawRectsGroup {
public:
    DrawRectsGroup(RenderBuffer&, AssetMetadata* texture, size_t capacity);

    size_t count() const { return m_count; }
    size_t capacity() const { return m_capacity; }

    // Makes space for `count` rects, and returns a pointer to the first one, so you can modify them later.
    // Guarantees that they're contiguous, but that means that there must be room for `count` items in the
    // group's current sub-group.
    // Basically, call this before adding anything, with count <= 255, and you'll be fine.
    Span<RenderItem_DrawRects_Item> reserve(size_t reserve_count);

    void add_rect(Rect2 bounds, Colour color, Rect2 uv);
    void add_glyph_rect(BitmapFontGlyph const& glyph, V2 position, Colour color);
    void add_sprite_rect(Sprite const& sprite, Rect2 bounds, Colour color);
    void add_untextured_rect(Rect2 bounds, Colour color);
    void offset_range(s32 startIndex, s32 endIndexInclusive, float offsetX, float offsetY);

    void end();

private:
    struct SubGroup {
        RenderItem_DrawRects* header;
        RenderItem_DrawRects_Item* first;
        s32 capacity;

        SubGroup* prev;
        SubGroup* next;
    };

    SubGroup begin_sub_group();
    void end_current_sub_group();

    RenderBuffer& m_buffer;

    SubGroup m_first_sub_group;
    SubGroup* m_current_sub_group;

    size_t m_count;
    size_t m_capacity;

    AssetMetadata* m_texture;
};
