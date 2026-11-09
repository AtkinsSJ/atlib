/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DrawRectsGroup.h"

#include <Debug/Debug.h>
#include <Gfx/BitmapFont.h>
#include <Gfx/RenderBuffer.h>
#include <Gfx/Sprite.h>

DrawRectsGroup::DrawRectsGroup(RenderBuffer& buffer, AssetMetadata* texture, size_t capacity)
    : m_buffer(buffer)
    , m_count(0)
    , m_capacity(capacity)
    , m_texture(texture)
{
    m_first_sub_group = begin_sub_group();
    m_current_sub_group = &m_first_sub_group;
}

Span<RenderItem_DrawRects_Item> DrawRectsGroup::reserve(size_t reserve_count)
{
    ASSERT((m_current_sub_group->header->count + reserve_count) <= m_current_sub_group->capacity);

    RenderItem_DrawRects_Item* result = m_current_sub_group->first + m_current_sub_group->header->count;

    m_current_sub_group->header->count = static_cast<u8>(m_current_sub_group->header->count + reserve_count);
    m_count += reserve_count;

    return Span { reserve_count, result };
}

void DrawRectsGroup::add_rect(Rect2 bounds, Colour color, Rect2 uv)
{
    ASSERT(m_count < m_capacity);

    if (m_current_sub_group->header->count == m_current_sub_group->capacity) {
        end_current_sub_group();
        SubGroup* prevSubGroup = m_current_sub_group;
        m_current_sub_group = temp_arena().allocate<SubGroup>();
        *m_current_sub_group = begin_sub_group();
        prevSubGroup->next = m_current_sub_group;
        m_current_sub_group->prev = prevSubGroup;
    }
    ASSERT(m_current_sub_group->header->count < m_current_sub_group->capacity);

    RenderItem_DrawRects_Item* item = m_current_sub_group->first + m_current_sub_group->header->count++;
    item->bounds = bounds;
    item->color = color;
    item->uv = uv;

    m_count++;
}

void DrawRectsGroup::add_glyph_rect(BitmapFontGlyph const& glyph, V2 position, Colour color)
{
    Rect2 bounds { position.x + glyph.xOffset, position.y + glyph.yOffset, static_cast<float>(glyph.width), static_cast<float>(glyph.height) };
    add_rect(bounds, color, glyph.uv);
}

void DrawRectsGroup::add_sprite_rect(Sprite const& sprite, Rect2 bounds, Colour color)
{
    ASSERT(m_texture == sprite.texture);

    add_rect(bounds, color, sprite.uv);
}

void DrawRectsGroup::add_untextured_rect(Rect2 bounds, Colour color)
{
    add_rect(bounds, color, { 0, 0, 0, 0 });
}

void DrawRectsGroup::offset_range(s32 startIndex, s32 endIndexInclusive, float offsetX, float offsetY)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Renderer);

    ASSERT(startIndex >= 0 && startIndex < m_count);
    ASSERT(endIndexInclusive >= 0 && endIndexInclusive < m_count);
    ASSERT(startIndex <= endIndexInclusive);

    V2 offset { offsetX, offsetY };

    s32 debugItemsUpdated = 0;

    SubGroup* subGroup = &m_first_sub_group;
    s32 firstIndexInSubGroup = 0;
    // Find the first subgroup that's involved
    while (firstIndexInSubGroup + subGroup->header->count < startIndex) {
        firstIndexInSubGroup += subGroup->header->count;
        subGroup = subGroup->next;
    }

    // Update the items in the range
    while (firstIndexInSubGroup <= endIndexInclusive) {
        s32 index = startIndex - firstIndexInSubGroup;
        if (index < 0)
            index = 0;
        for (;
            (index <= endIndexInclusive - firstIndexInSubGroup) && (index < subGroup->header->count);
            index++) {
            RenderItem_DrawRects_Item* item = subGroup->first + index;
            item->bounds.translate(offset);
            debugItemsUpdated++;
        }

        firstIndexInSubGroup += subGroup->header->count;
        subGroup = subGroup->next;
    }

    ASSERT(debugItemsUpdated == (endIndexInclusive - startIndex) + 1);
}

DrawRectsGroup::SubGroup DrawRectsGroup::begin_sub_group()
{
    SubGroup result = {};

    s32 sub_group_item_count = min(m_capacity - m_count, (s32)maxRenderItemsPerGroup);
    ASSERT(sub_group_item_count > 0 && sub_group_item_count <= maxRenderItemsPerGroup);

    size_t reserved_size = sizeof(RenderItem_DrawRects_Item) * sub_group_item_count;
    auto [item, data] = m_buffer.append_render_item_with_reserved_data<RenderItem_DrawRects>(RenderItemType::DrawRects, reserved_size);

    result.header = item;
    result.first = reinterpret_cast<RenderItem_DrawRects_Item*>(data.raw_data());
    result.capacity = sub_group_item_count;

    *result.header = {};
    result.header->count = 0;

    return result;
}

void DrawRectsGroup::end_current_sub_group()
{
    m_buffer.consume_reserved_memory(m_current_sub_group->header->count * sizeof(RenderItem_DrawRects_Item));
}

void DrawRectsGroup::end()
{
    end_current_sub_group();
}
