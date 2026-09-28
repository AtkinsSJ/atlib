/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Palette.h"
#include <Assets/AssetManager.h>
#include <Assets/AssetRef.h>
#include <Assets/ContainerAsset.h>
#include <IO/LineReader.h>
#include <Util/Lexer.h>

Palette::Palette(Type type, Array<Colour> colours)
    : m_type(type)
    , m_colours(move(colours))
{
}

ErrorOr<OwnedRef<Asset>> Palette::load_defs(AssetMetadata& metadata, Blob file_data)
{
    LineReader reader { metadata.shortName, file_data };

    struct PaletteData {
        StringView name;
        Type type { Type::Fixed };
        size_t size { 0 };
        ChunkedArray<Colour> fixed_colors;
        Optional<Colour> from_color;
        Optional<Colour> to_color;
    };
    ChunkedArray<PaletteData> palettes { temp_arena(), 128 };
    while (reader.load_next_line()) {
        Lexer lexer { reader.current_line() };

        // Commands
        if (lexer.consume_specific(':')) {
            // Define something
            auto command = lexer.consume_token();
            lexer.discard_whitespace();

            if (command == "Palette"_s) {
                auto palette_name = lexer.consume_token();
                lexer.discard_whitespace();
                if (!palette_name.has_value() || lexer.has_next())
                    return reader.make_error_message("Invalid :Palette definition: Expected `:Palette NAME`"_s);
                palettes.append(PaletteData {
                    .name = palette_name.release_value(),
                    .fixed_colors = { temp_arena(), 128 },
                });
            } else {
                return reader.make_error_message("Only :Palette definitions are allowed here!"_s);
            }
            continue;
        }

        auto maybe_property = lexer.consume_token();
        if (!maybe_property.has_value())
            continue;
        auto property_name = maybe_property.release_value();
        lexer.discard_whitespace();

        if (palettes.is_empty())
            return reader.make_error_message("Found a property before starting a :Palette"_s);

        auto read_colour_property = [](StringView property_name, LineReader& reader, Lexer& lexer) -> ErrorOr<Colour> {
            auto colour = Colour::read(lexer);
            lexer.discard_whitespace();
            if (colour.is_error())
                return reader.make_error_message("Failed to parse {0}: {1}"_s, { property_name, colour.release_error() });
            if (lexer.has_next())
                return reader.make_error_message("Failed to parse {0}. Expected: `{0} COLOR`"_s, { property_name });
            return colour.release_value();
        };

        auto& palette = palettes.get(palettes.count - 1);
        if (property_name == "type"_s) {
            auto type = lexer.consume_token();
            lexer.discard_whitespace();
            if (!type.has_value() || lexer.has_next())
                return reader.make_error_message("Failed to parse type. Expected: `type NAME`"_s);

            if (type == "fixed"_s) {
                palette.type = Type::Fixed;
            } else if (type == "gradient"_s) {
                palette.type = Type::Gradient;
            } else {
                return reader.make_error_message("Unrecognised palette type '{0}', allowed values are: fixed, gradient"_s, { type.value() });
            }
        } else if (property_name == "size"_s) {
            auto size = lexer.consume_int<size_t>();
            lexer.discard_whitespace();
            if (!size.has_value() || lexer.has_next())
                return reader.make_error_message("Failed to parse size. Expected: `size INTEGER`"_s);
            palette.size = size.release_value();
        } else if (property_name == "color"_s) {
            auto colour = read_colour_property(property_name, reader, lexer);
            if (colour.is_error())
                return colour.release_error();
            if (palette.type != Type::Fixed)
                return reader.make_error_message("'color' is only a valid command for fixed palettes."_s);

            s32 color_index = palette.fixed_colors.count;
            // FIXME: Is `size` actually necessary/useful?
            if (color_index >= palette.size)
                return reader.make_error_message("Too many 'color' definitions! 'size' must be large enough."_s);
            palette.fixed_colors.append(colour.release_value());
        } else if (property_name == "from"_s) {
            auto from_colour = read_colour_property(property_name, reader, lexer);
            if (from_colour.is_error())
                return from_colour.release_error();
            if (palette.type != Type::Gradient)
                return reader.make_error_message("'from' is only a valid command for gradient palettes."_s);

            palette.from_color = from_colour.release_value();
        } else if (property_name == "to"_s) {
            auto to_colour = read_colour_property(property_name, reader, lexer);
            if (to_colour.is_error())
                return to_colour.release_error();
            if (palette.type != Type::Gradient)
                return reader.make_error_message("'to' is only a valid command for gradient palettes."_s);

            palette.to_color = to_colour.release_value();
        } else {
            return reader.make_error_message("Unrecognised property '{0}'"_s, { property_name });
        }
    }

    // Load all the palettes, now that we know their properties are all set.
    auto children = asset_manager().allocate_array<GenericAssetRef>(palettes.count);

    for (auto it = palettes.iterate(); it.hasNext(); it.next()) {
        auto& palette = it.get();

        switch (palette.type) {
        case Type::Gradient: {
            auto colours_array = asset_manager().allocate_filled_array<Colour>(palette.size);

            float ratio = 1.0f / static_cast<float>(palette.size);
            for (auto i = 0u; i < palette.size; i++) {
                colours_array[i] = lerp(palette.from_color.value(), palette.to_color.value(), i * ratio);
            }

            auto& palette_metadata = *asset_manager().add_asset(asset_type(), palette.name, {});
            palette_metadata.loaded_asset = adopt_own(*new Palette(palette.type, move(colours_array)));
            palette_metadata.state = AssetMetadata::State::Loaded;
            children.append(palette_metadata.get_ref());
        } break;

        case Type::Fixed: {
            auto colours_array = asset_manager().allocate_filled_array<Colour>(palette.fixed_colors.count);
            for (auto i = 0u; i < colours_array.count(); i++)
                colours_array[i] = palette.fixed_colors.get(i);

            auto& palette_metadata = *asset_manager().add_asset(asset_type(), palette.name, {});
            palette_metadata.loaded_asset = adopt_own(*new Palette(palette.type, move(colours_array)));
            palette_metadata.state = AssetMetadata::State::Loaded;
            children.append(palette_metadata.get_ref());
        } break;
        }
    }

    return { adopt_own(*new ContainerAsset(move(children))) };
}

Colour Palette::colour_at(size_t index) const
{
    return m_colours[index];
}

Colour Palette::first() const
{
    return m_colours.first();
}

Colour Palette::last() const
{
    return m_colours.last();
}

size_t Palette::size() const
{
    return m_colours.count();
}

void Palette::unload(AssetMetadata&)
{
    asset_manager().deallocate(m_colours);
}
