/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Alignment.h"

#include <Util/ErrorOr.h>
#include <Util/Lexer.h>
#include <Util/String.h>

ErrorOr<Alignment> Alignment::read(Lexer& lexer)
{
    return lexer.consume_with_callback_or_error<Alignment>([](Lexer& lexer) -> ErrorOr<Alignment> {
        Optional<HAlign> h;
        Optional<VAlign> v;

        while (lexer.has_next()) {
            auto token = lexer.consume_token();
            if (!token.has_value())
                break;
            lexer.discard_whitespace();

            if (token == "LEFT"_s) {
                if (h.has_value())
                    return "Multiple horizontal alignment keywords given!"_s;
                h = HAlign::Left;
            } else if (token == "H_CENTRE"_s) {
                if (h.has_value())
                    return "Multiple horizontal alignment keywords given!"_s;
                h = HAlign::Centre;
            } else if (token == "RIGHT"_s) {
                if (h.has_value())
                    return "Multiple horizontal alignment keywords given!"_s;
                h = HAlign::Right;
            } else if (token == "EXPAND_H"_s) {
                if (h.has_value())
                    return "Multiple horizontal alignment keywords given!"_s;
                h = HAlign::Fill;
            } else if (token == "TOP"_s) {
                if (v.has_value())
                    return "Multiple vertical alignment keywords given!"_s;
                v = VAlign::Top;
            } else if (token == "V_CENTRE"_s) {
                if (v.has_value())
                    return "Multiple vertical alignment keywords given!"_s;
                v = VAlign::Centre;
            } else if (token == "BOTTOM"_s) {
                if (v.has_value())
                    return "Multiple vertical alignment keywords given!"_s;
                v = VAlign::Bottom;
            } else if (token == "EXPAND_V"_s) {
                if (v.has_value())
                    return "Multiple vertical alignment keywords given!"_s;
                v = VAlign::Fill;
            } else {
                return myprintf("Unrecognized alignment keyword '{0}'"_s, { token.value() });
            }

            if (h.has_value() && v.has_value())
                return Alignment { h.release_value(), v.release_value() };
        }

        if (h.has_value() && v.has_value())
            return Alignment { h.release_value(), v.release_value() };

        return "Either one or both alignment keywords are missing"_s;
    });
}
