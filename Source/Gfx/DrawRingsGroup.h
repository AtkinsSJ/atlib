/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/RenderItem.h>

struct DrawRingsSubGroup {
    RenderItem_DrawRings* header;
    RenderItem_DrawRings_Item* first;
    s32 maxCount;

    DrawRingsSubGroup* prev;
    DrawRingsSubGroup* next;
};

// Think of this like a "handle". It has data inside but you shouldn't touch it from user code!
struct DrawRingsGroup {

    void add_ring(V2 centre, float radius, float thickness, Colour color);

    DrawRingsSubGroup begin_sub_group();
    void end_current_sub_group();

    void end();

    RenderBuffer* buffer;

    DrawRingsSubGroup firstSubGroup;
    DrawRingsSubGroup* currentSubGroup;

    s32 count;
    s32 maxCount;
};
