/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DrawRingsGroup.h"

#include <Gfx/Renderer.h>

DrawRingsGroup::DrawRingsGroup(RenderBuffer& buffer, size_t capacity)
    : m_buffer(buffer)
    , m_count(0)
    , m_capacity(capacity)
{
    m_first_sub_group = begin_sub_group();
    m_current_sub_group = &m_first_sub_group;
}

void DrawRingsGroup::add_ring(V2 centre, float radius, float thickness, Colour color)
{
    // @Copypasta DrawRectsGroup::add_rect() - maybe factor out common "grouped render items" code?
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

    RenderItem_DrawRings_Item* item = m_current_sub_group->first + m_current_sub_group->header->count++;
    item->centre = centre;
    item->radius = radius;
    item->thickness = thickness;
    item->color = color;

    m_count++;
}

DrawRingsGroup::SubGroup DrawRingsGroup::begin_sub_group()
{
    // @Copypasta DrawRectsGroup::begin_sub_group() - maybe factor out common "grouped render items" code?
    SubGroup result = {};

    s32 sub_group_item_count = min(m_capacity - m_count, (s32)maxRenderItemsPerGroup);
    ASSERT(sub_group_item_count > 0 && sub_group_item_count <= maxRenderItemsPerGroup);

    smm reserved_size = sizeof(RenderItem_DrawRings_Item) * sub_group_item_count;
    auto [item, data] = m_buffer.append_render_item_with_reserved_data<RenderItem_DrawRings>(RenderItemType::DrawRings, reserved_size);

    result.header = item;
    result.first = reinterpret_cast<RenderItem_DrawRings_Item*>(data.raw_data());
    result.capacity = sub_group_item_count;

    *result.header = {};
    result.header->count = 0;

    return result;
}

void DrawRingsGroup::end_current_sub_group()
{
    m_buffer.consume_reserved_memory(m_current_sub_group->header->count * sizeof(RenderItem_DrawRings_Item));
}

void DrawRingsGroup::end()
{
    end_current_sub_group();
}
