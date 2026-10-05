/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/String.h>

namespace Assets {

class TextRef {
public:
    explicit TextRef(String short_name)
        : m_name(move(short_name))
    {
    }

    String const& name() const { return m_name; }
    String const& text() const;

protected:
    String m_name;
    mutable Optional<String> m_text;
    mutable u32 m_asset_generation { 0 };
};

}
