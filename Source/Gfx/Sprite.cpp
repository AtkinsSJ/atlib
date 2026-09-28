/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Sprite.h"
#include <Assets/AssetManager.h>
#include <Assets/AssetRef.h>
#include <Assets/ContainerAsset.h>
#include <Gfx/Ninepatch.h>
#include <Gfx/Texture.h>
#include <IO/LineReader.h>
#include <Util/Lexer.h>

Sprite& Sprite::get(StringView name)
{
    return SpriteGroup::get(name).sprites[0];
}

OwnedRef<SpriteGroup> SpriteGroup::make_placeholder()
{
    auto sprite_group = adopt_own(*new SpriteGroup);
    sprite_group->sprites = asset_manager().allocate_array<Sprite>(1);
    sprite_group->sprites.append({
        .texture = &asset_manager().get_placeholder_asset(Texture::asset_type()),
        .uv = { 0.0f, 0.0f, 1.0f, 1.0f },
    });

    return sprite_group;
}

void SpriteGroup::unload(AssetMetadata&)
{
    asset_manager().deallocate(sprites);
}

Sprite& SpriteRef::get() const
{
    if (asset_manager().asset_generation() > m_asset_generation) {
        auto& group = SpriteGroup::get(m_sprite_group_name);
        m_pointer = &group.sprites[m_sprite_index % group.sprites.count()];
        m_asset_generation = asset_manager().asset_generation();
    }

    return *m_pointer;
}

static AssetMetadata* add_sprite_group(StringView name, s32 spriteCount)
{
    ASSERT(spriteCount > 0); // Must have a positive number of sprites in a Sprite Group!

    AssetMetadata* metadata = asset_manager().add_asset(SpriteGroup::asset_type(), name, {});
    auto asset = adopt_own(*new SpriteGroup);
    asset->sprites = asset_manager().allocate_array<Sprite>(spriteCount);

    metadata->loaded_asset = move(asset);
    metadata->state = AssetMetadata::State::Loaded;
    return metadata;
}

static AssetMetadata* add_ninepatch(StringView name, StringView filename, s32 pu0, s32 pu1, s32 pu2, s32 pu3, s32 pv0, s32 pv1, s32 pv2, s32 pv3)
{
    AssetMetadata* texture_metadata = asset_manager().add_asset(Texture::asset_type(), filename);
    texture_metadata->ensure_is_loaded();

    AssetMetadata* metadata = asset_manager().add_asset(Ninepatch::asset_type(), name, {});
    metadata->loaded_asset = adopt_own(*new Ninepatch(*texture_metadata, pu0, pu1, pu2, pu3, pv0, pv1, pv2, pv3));
    metadata->state = AssetMetadata::State::Loaded;
    return metadata;
}

ErrorOr<OwnedRef<Asset>> load_sprite_defs(AssetMetadata& metadata, Blob data)
{
    LineReader reader { metadata.shortName, data };

    AssetMetadata* texture_asset = nullptr;
    V2I sprite_size = v2i(0, 0);
    V2I sprite_border = v2i(0, 0);
    AssetMetadata* current_sprite_group_metadata = nullptr;

    // Count the number of child assets, so we can allocate our spriteNames array
    size_t child_asset_count = 0;
    while (reader.load_next_line()) {
        if (reader.current_line().starts_with(':'))
            child_asset_count++;
    }
    auto children = asset_manager().allocate_array<GenericAssetRef>(child_asset_count);
    reader.restart();

    // Now, actually read things
    while (reader.load_next_line()) {
        Lexer lexer { reader.current_line() };

        // Commands
        if (lexer.consume_specific(':')) {
            // Define something
            auto command = lexer.consume_token();
            lexer.discard_whitespace();

            texture_asset = nullptr;
            current_sprite_group_metadata = nullptr;

            if (command == "Ninepatch"_s) {
                auto name = lexer.consume_token();
                lexer.discard_whitespace();
                auto filename = lexer.consume_token();
                lexer.discard_whitespace();
                auto pu0 = lexer.consume_int<s32>();
                lexer.discard_whitespace();
                auto pu1 = lexer.consume_int<s32>();
                lexer.discard_whitespace();
                auto pu2 = lexer.consume_int<s32>();
                lexer.discard_whitespace();
                auto pu3 = lexer.consume_int<s32>();
                lexer.discard_whitespace();
                auto pv0 = lexer.consume_int<s32>();
                lexer.discard_whitespace();
                auto pv1 = lexer.consume_int<s32>();
                lexer.discard_whitespace();
                auto pv2 = lexer.consume_int<s32>();
                lexer.discard_whitespace();
                auto pv3 = lexer.consume_int<s32>();
                lexer.discard_whitespace();

                if (!all_have_values(name, filename, pu0, pu1, pu2, pu3, pv0, pv1, pv2, pv3) || lexer.has_next()) {
                    return reader.make_error_message("Couldn't parse Ninepatch. Expected: ':Ninepatch identifier filename.png pu0 pu1 pu2 pu3 pv0 pv1 pv2 pv3'"_s);
                }

                AssetMetadata* ninepatch = add_ninepatch(name.release_value(), filename.release_value(), pu0.release_value(), pu1.release_value(), pu2.release_value(), pu3.release_value(), pv0.release_value(), pv1.release_value(), pv2.release_value(), pv3.release_value());

                children.append(ninepatch->get_ref());
            } else if (command == "Sprite"_s) {
                // @Copypasta from the SpriteGroup branch, and the 'sprite' property
                auto name = lexer.consume_token();
                lexer.discard_whitespace();
                auto filename = lexer.consume_token();
                lexer.discard_whitespace();
                auto sprite_size_in = V2I::read_size(lexer);
                lexer.discard_whitespace();

                if (!all_have_values(name, filename, sprite_size_in) || lexer.has_next()) {
                    return reader.make_error_message("Couldn't parse Sprite. Expected: ':Sprite identifier filename.png SWxSH'"_s);
                }

                sprite_size = sprite_size_in.release_value();

                AssetMetadata* group = add_sprite_group(name.release_value(), 1);
                auto& group_asset = dynamic_cast<SpriteGroup&>(*group->loaded_asset);

                group_asset.sprites.append({
                    .texture = asset_manager()
                        .add_asset(Texture::asset_type(), filename.release_value()),
                    .uv = { 0, 0, sprite_size.x, sprite_size.y },
                    .pixelWidth = sprite_size.x,
                    .pixelHeight = sprite_size.y,
                });

                children.append(group->get_ref());
            } else if (command == "SpriteGroup"_s) {
                auto name = lexer.consume_token();
                lexer.discard_whitespace();
                auto filename = lexer.consume_token();
                lexer.discard_whitespace();
                auto sprite_size_in = V2I::read_size(lexer);
                lexer.discard_whitespace();

                if (!all_have_values(name, filename, sprite_size_in)) {
                    return reader.make_error_message("Couldn't parse SpriteGroup. Expected: ':SpriteGroup identifier filename.png SWxSH'"_s);
                }

                texture_asset = asset_manager().add_asset(Texture::asset_type(), filename.release_value());
                sprite_size = sprite_size_in.release_value();

                auto sprite_count = 0u;
                {
                    auto saved_position = reader.save_state();
                    while (reader.load_next_line()) {
                        Lexer variant_lexer { reader.current_line() };
                        if (variant_lexer.consume_specific(':'))
                            break; // Next command
                        if (variant_lexer.consume_token() == "sprite"_sv)
                            sprite_count++;
                    }
                    reader.restore_state(saved_position);
                }

                if (sprite_count < 1)
                    return reader.make_error_message("SpriteGroup must contain at least 1 sprite!"_s);
                current_sprite_group_metadata = add_sprite_group(name.release_value(), sprite_count);

                children.append(current_sprite_group_metadata->get_ref());
            } else {
                return reader.make_error_message("Unrecognised command. Only :Sprite and :SpriteGroup are supported."_s);
            }
            continue;
        }

        // Properties!
        auto maybe_property = lexer.consume_token();
        if (!maybe_property.has_value())
            continue;
        auto property_name = maybe_property.release_value();
        lexer.discard_whitespace();

        if (current_sprite_group_metadata == nullptr)
            return reader.make_error_message("Found a property outside of a :SpriteGroup!"_s);

        if (property_name == "border"_s) {
            auto border_size = V2I::read_size(lexer);
            lexer.discard_whitespace();

            if (!border_size.has_value() || lexer.has_next())
                return reader.make_error_message("Couldn't parse border. Expected 'border WIDTHxHEIGHT'."_s);

            sprite_border = border_size.release_value();
        } else if (property_name == "sprite"_s) {
            auto position = V2I::read_position(lexer);
            lexer.discard_whitespace();

            if (!position.has_value() || lexer.has_next())
                return reader.make_error_message("Couldn't parse {0}. Expected '{0} X,Y'."_s, { property_name });

            auto& group_asset = dynamic_cast<SpriteGroup&>(*current_sprite_group_metadata->loaded_asset);
            group_asset.sprites.append({
                .texture = texture_asset,
                .uv = { sprite_border.x + position.value().x * (sprite_size.x + sprite_border.x + sprite_border.x),
                    sprite_border.y + position.value().y * (sprite_size.y + sprite_border.y + sprite_border.y),
                    sprite_size.x, sprite_size.y },
                .pixelWidth = sprite_size.x,
                .pixelHeight = sprite_size.y,
            });
        } else {
            return reader.make_error_message("Unrecognised property '{0}'"_s, { property_name });
        }
    }

    // Load all the sprites, now that we know their properties are all set.
    for (auto& child : children) {
        auto& sprite_group_metadata = child.metadata();
        if (sprite_group_metadata.type != SpriteGroup::asset_type())
            continue;

        auto& sprite_group = dynamic_cast<SpriteGroup&>(*sprite_group_metadata.loaded_asset);

        // Convert UVs from pixel space to 0-1 space
        for (auto& sprite : sprite_group.sprites) {
            // FIXME: Should refer to textures some other way!
            AssetMetadata* texture_metadata = sprite.texture;
            texture_metadata->ensure_is_loaded();
            auto& texture = dynamic_cast<Texture&>(*texture_metadata->loaded_asset);
            float textureWidth = texture.surface->w;
            float textureHeight = texture.surface->h;

            sprite.uv = {
                sprite.uv.x() / textureWidth,
                sprite.uv.y() / textureHeight,
                sprite.uv.width() / textureWidth,
                sprite.uv.height() / textureHeight
            };
        }
    }

    return { adopt_own(*new ContainerAsset(move(children))) };
}
