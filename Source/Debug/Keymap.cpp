/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Keymap.h"

#include <Util/Lexer.h>

ErrorOr<OwnedRef<Keymap>> Keymap::load(AssetMetadata& metadata, Blob file_data)
{
    // Separate reader just for the command count.
    auto command_count = 0u;
    {
        LineReader reader { metadata.shortName, file_data };
        while (reader.load_next_line()) {
            if (!reader.current_line().is_empty())
                command_count++;
        }
    }

    auto& assets = asset_manager();
    auto data = assets.allocate_blob(file_data);
    auto shortcuts = assets.allocate_array<CommandShortcut>(command_count);

    // Now we create a reader on the stored copy of the file data, so that we can point StringViews into it.
    LineReader reader { metadata.shortName, data.sub_blob(0, file_data.size()) };
    while (reader.load_next_line()) {
        Lexer lexer { reader.current_line() };
        auto shortcut = KeyboardShortcut::read(lexer);
        lexer.discard_whitespace();

        if (shortcut.is_error())
            return reader.make_error_message("Failed to read keyboard shortcut: {}"_s, { shortcut.release_error() });

        auto command = lexer.consume_remainder();
        if (!command.has_value())
            return reader.make_error_message("Missing command."_s);

        shortcuts.append({
            .shortcut = shortcut.release_value(),
            .command = command.release_value(),
        });
    }

    return adopt_own(*new Keymap(move(data), move(shortcuts)));
}

Keymap::Keymap(Blob data, Array<CommandShortcut> shortcuts)
    : m_data(move(data))
    , m_shortcuts(move(shortcuts))
{
}

void Keymap::unload(AssetMetadata&)
{
    auto& assets = asset_manager();
    assets.deallocate(m_data);
    assets.deallocate(m_shortcuts);
}
