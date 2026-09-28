/*
 * Copyright (c) 2025-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Padding.h"

#include <Util/Lexer.h>

Optional<Padding> Padding::read(Lexer& lexer)
{
    // Padding definitions may be 1, 2, 3 or 4 values, as CSS does it:
    //   All
    //   Vertical Horizontal
    //   Top Horizontal Bottom
    //   Top Right Bottom Left
    // So, clockwise from the top, with sensible fallbacks

    return lexer.consume_with_callback<Padding>([](Lexer& lexer) -> Optional<Padding> {
        lexer.discard_whitespace();
        auto top = lexer.consume_int<s32>();
        lexer.discard_whitespace();
        auto right = lexer.consume_int<s32>();
        lexer.discard_whitespace();
        auto bottom = lexer.consume_int<s32>();
        lexer.discard_whitespace();
        auto left = lexer.consume_int<s32>();
        lexer.discard_whitespace();

        if (!top.has_value())
            return {};

        if (left.has_value()) {
            return Padding {
                .top = top.value(),
                .bottom = bottom.value(),
                .left = left.value(),
                .right = right.value(),
            };
        }

        if (bottom.has_value()) {
            return Padding {
                .top = top.value(),
                .bottom = bottom.value(),
                .left = right.value(),
                .right = right.value(),
            };
        }

        if (right.has_value()) {
            return Padding {
                .top = top.value(),
                .bottom = top.value(),
                .left = right.value(),
                .right = right.value(),
            };
        }

        return Padding {
            .top = top.value(),
            .bottom = top.value(),
            .left = top.value(),
            .right = top.value(),
        };
    });
}
