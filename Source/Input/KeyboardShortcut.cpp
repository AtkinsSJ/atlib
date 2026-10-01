/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "KeyboardShortcut.h"

#include <Input/Input.h>
#include <Util/Lexer.h>

/**
 * NB: Right now, we only support a very small number of shortcut key types.
 * Add fancier stuff as needed.
 *
 * Basic format of a shortcut is, eg:
 * Ctrl+Alt+Shift+Super+Home
 *
 * If any key in the sequence is unrecognised, we return SDLK_UNKNOWN for the `key` field.
 *
 * Only one "key", plus any combination of modifiers, is supported.
 * Note that this means you can't bind something to just pressing a modifier key!
 */

KeyboardShortcut::KeyboardShortcut(SDL_Keycode key, Flags<ModifierKey>&& modifiers)
    : m_key(key)
    , m_modifiers(move(modifiers))
{
}

ErrorOr<KeyboardShortcut> KeyboardShortcut::read(Lexer& lexer)
{
    return lexer.consume_with_callback_or_error<KeyboardShortcut>([](Lexer& lexer) -> ErrorOr<KeyboardShortcut> {
        Flags<ModifierKey> modifiers;
        while (lexer.has_next()) {
            auto key_name = lexer.consume_until([](auto c) {
                return is_ascii_whitespace(c) || c == '+';
            });
            if (!key_name.has_value())
                return "Missing key name"_s;

            if (auto modifier = modifier_key_from_string(key_name.value()); modifier.has_value()) {
                if (modifiers.has(modifier.value()))
                    return myprintf("Duplicate modifier key `{}` in shortcut."_s, { key_name.value() });
                modifiers.add(modifier.value());

                if (!lexer.consume_specific('+'))
                    return "Missing a key: Modifier-key-only shortcuts are not supported."_s;

                continue;
            }

            auto key_string = key_name.value().deprecated_to_string();
            auto found_key = input_state().key_by_name.get(key_string);
            if (!found_key.has_value())
                return myprintf("Unrecognised key name '{}'."_s, { key_name.value() });
            if (lexer.peek() == '+')
                return "Only a single non-modifier key per shortcut is supported."_s;

            return KeyboardShortcut { found_key.release_value(), move(modifiers) };
        }
        return "Input string is empty."_s;
    });
}

bool KeyboardShortcut::was_just_pressed() const
{
    return keyJustPressed(m_key, m_modifiers, true);
}
