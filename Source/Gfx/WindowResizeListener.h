/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

class WindowResizeListener {
public:
    virtual ~WindowResizeListener() = default;
    virtual void on_window_resized(u32 width, u32 height) = 0;
};
