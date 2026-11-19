/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "TextInputTarget.h"

#include <Gfx/Renderer.h>
#include <Input/Input.h>

void TextInputTarget::capture_input()
{
    auto& input = input_state();
    input.m_text_input_target = Ref { *this };
}

void TextInputTarget::release_input()
{
    auto& input = input_state();
    if (input.m_text_input_target == *this) {
        input.m_text_input_target.clear();
    }
}

bool TextInputTarget::has_captured_input() const
{
    return input_state().m_text_input_target == const_cast<TextInputTarget&>(*this);
}
