/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/RenderItem.h>

// Think of this like a "handle". It has data inside but you shouldn't touch it from user code!
class DrawRingsGroup {
public:
    DrawRingsGroup(RenderBuffer&, size_t capacity);

    size_t count() const { return m_count; }
    size_t capacity() const { return m_capacity; }

    void add_ring(V2 centre, float radius, float thickness, Colour color);

    void end();

private:
    struct SubGroup {
        RenderItem_DrawRings* header;
        RenderItem_DrawRings_Item* first;
        s32 capacity;

        SubGroup* prev;
        SubGroup* next;
    };

    SubGroup begin_sub_group();
    void end_current_sub_group();

    RenderBuffer& m_buffer;

    SubGroup m_first_sub_group;
    SubGroup* m_current_sub_group;

    s32 m_count;
    s32 m_capacity;
};
