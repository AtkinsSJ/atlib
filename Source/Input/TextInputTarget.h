/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

class TextInputTarget {
public:
    virtual ~TextInputTarget() = default;

    void capture_input();
    void release_input();
    bool has_captured_input() const;
};
