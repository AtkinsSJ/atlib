/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "TextRef.h"

#include <Assets/AssetManager.h>

namespace Assets {

String const& TextRef::text() const
{
    auto& assets = asset_manager();
    if (!m_text.has_value() || assets.asset_generation() > m_asset_generation) {
        m_text = assets.get_text(m_name);
        m_asset_generation = assets.asset_generation();
    }

    return m_text.value();
}

}
