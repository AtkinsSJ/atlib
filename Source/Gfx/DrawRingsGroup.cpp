/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DrawRingsGroup.h"

#include <Gfx/Renderer.h>
#include <Util/MemoryArena.h>

void DrawRingsGroup::add_ring(V2 centre, float radius, float thickness, Colour color)
{
    // @Copypasta DrawRectsGroup::add_rect() - maybe factor out common "grouped render items" code?
    ASSERT(count < maxCount);

    if (currentSubGroup->header->count == currentSubGroup->maxCount) {
        end_current_sub_group();
        DrawRingsSubGroup* prevSubGroup = currentSubGroup;
        currentSubGroup = temp_arena().allocate<DrawRingsSubGroup>();
        *currentSubGroup = begin_sub_group();
        prevSubGroup->next = currentSubGroup;
        currentSubGroup->prev = prevSubGroup;
    }
    ASSERT(currentSubGroup->header->count < currentSubGroup->maxCount);

    RenderItem_DrawRings_Item* item = currentSubGroup->first + currentSubGroup->header->count++;
    item->centre = centre;
    item->radius = radius;
    item->thickness = thickness;
    item->color = color;

    count++;
}

DrawRingsSubGroup DrawRingsGroup::begin_sub_group()
{
    // @Copypasta DrawRectsGroup::begin_sub_group() - maybe factor out common "grouped render items" code?
    DrawRingsSubGroup result = {};

    s32 sub_group_item_count = min(maxCount - count, (s32)maxRenderItemsPerGroup);
    ASSERT(sub_group_item_count > 0 && sub_group_item_count <= maxRenderItemsPerGroup);

    smm reserved_size = sizeof(RenderItem_DrawRings_Item) * sub_group_item_count;
    auto [item, data] = buffer->append_render_item_with_reserved_data<RenderItem_DrawRings>(RenderItemType::DrawRings, reserved_size);
    buffer->hasRangeReserved = true;

    result.header = item;
    result.first = reinterpret_cast<RenderItem_DrawRings_Item*>(data.raw_data());
    result.maxCount = sub_group_item_count;

    *result.header = {};
    result.header->count = 0;

    return result;
}

void DrawRingsGroup::end_current_sub_group()
{
    // @Copypasta DrawRectsGroup::end_current_sub_group() - maybe factor out common "grouped render items" code?
    ASSERT(buffer->hasRangeReserved); // Attempted to finish a range while a range is not reserved!
    buffer->hasRangeReserved = false;

    buffer->currentChunk->used += currentSubGroup->header->count * sizeof(RenderItem_DrawRings_Item);
}

void DrawRingsGroup::end()
{
    end_current_sub_group();
}
