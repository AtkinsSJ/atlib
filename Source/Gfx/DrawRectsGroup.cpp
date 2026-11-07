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

Span<RenderItem_DrawRects_Item> DrawRectsGroup::reserve(size_t reserve_count)
{
    ASSERT((currentSubGroup->header->count + reserve_count) <= currentSubGroup->maxCount);

    RenderItem_DrawRects_Item* result = currentSubGroup->first + currentSubGroup->header->count;

    currentSubGroup->header->count = static_cast<u8>(currentSubGroup->header->count + reserve_count);
    count += reserve_count;

    return Span { reserve_count, result };
}

void DrawRectsGroup::add_rect(Rect2 bounds, Colour color, Rect2 uv)
{
    ASSERT(count < maxCount);

    if (currentSubGroup->header->count == currentSubGroup->maxCount) {
        end_current_sub_group();
        DrawRectsSubGroup* prevSubGroup = currentSubGroup;
        currentSubGroup = temp_arena().allocate<DrawRectsSubGroup>();
        *currentSubGroup = begin_sub_group();
        prevSubGroup->next = currentSubGroup;
        currentSubGroup->prev = prevSubGroup;
    }
    ASSERT(currentSubGroup->header->count < currentSubGroup->maxCount);

    RenderItem_DrawRects_Item* item = currentSubGroup->first + currentSubGroup->header->count++;
    item->bounds = bounds;
    item->color = color;
    item->uv = uv;

    count++;
}

void DrawRectsGroup::add_glyph_rect(BitmapFontGlyph const& glyph, V2 position, Colour color)
{
    Rect2 bounds { position.x + glyph.xOffset, position.y + glyph.yOffset, static_cast<float>(glyph.width), static_cast<float>(glyph.height) };
    add_rect(bounds, color, glyph.uv);
}

void DrawRectsGroup::add_sprite_rect(Sprite const& sprite, Rect2 bounds, Colour color)
{
    ASSERT(texture == sprite.texture);

    add_rect(bounds, color, sprite.uv);
}

void DrawRectsGroup::add_untextured_rect(Rect2 bounds, Colour color)
{
    add_rect(bounds, color, { 0, 0, 0, 0 });
}

void DrawRectsGroup::offset_range(s32 startIndex, s32 endIndexInclusive, float offsetX, float offsetY)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Renderer);

    ASSERT(startIndex >= 0 && startIndex < count);
    ASSERT(endIndexInclusive >= 0 && endIndexInclusive < count);
    ASSERT(startIndex <= endIndexInclusive);

    V2 offset { offsetX, offsetY };

    s32 debugItemsUpdated = 0;

    DrawRectsSubGroup* subGroup = &firstSubGroup;
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

DrawRectsSubGroup DrawRectsGroup::begin_sub_group()
{
    DrawRectsSubGroup result = {};

    s32 sub_group_item_count = min(maxCount - count, (s32)maxRenderItemsPerGroup);
    ASSERT(sub_group_item_count > 0 && sub_group_item_count <= maxRenderItemsPerGroup);

    size_t reserved_size = sizeof(RenderItem_DrawRects_Item) * sub_group_item_count;
    auto [item, data] = buffer->append_render_item_with_reserved_data<RenderItem_DrawRects>(RenderItemType::DrawRects, reserved_size);
    buffer->hasRangeReserved = true;

    result.header = item;
    result.first = reinterpret_cast<RenderItem_DrawRects_Item*>(data.raw_data());
    result.maxCount = sub_group_item_count;

    *result.header = {};
    result.header->count = 0;

    return result;
}

void DrawRectsGroup::end_current_sub_group()
{
    ASSERT(buffer->hasRangeReserved); // Attempted to finish a range while a range is not reserved!
    buffer->hasRangeReserved = false;
    buffer->currentChunk->used += currentSubGroup->header->count * sizeof(RenderItem_DrawRects_Item);
}

void DrawRectsGroup::end()
{
    end_current_sub_group();
}
