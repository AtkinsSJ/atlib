/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Optional.h>
#include <Util/StringView.h>

enum class ModifierKey : u8 {
    Alt,
    Ctrl,
    Shift,
    Super,
    COUNT,
};

constexpr Optional<ModifierKey> modifier_key_from_string(StringView const& key_name)
{
    if (key_name == "Alt"_sv)
        return ModifierKey::Alt;
    if (key_name == "Ctrl"_sv)
        return ModifierKey::Ctrl;
    if (key_name == "Shift"_sv)
        return ModifierKey::Shift;
    if (key_name == "Super"_sv)
        return ModifierKey::Super;
    return {};
}
