/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Drawable.h"
#include <Assets/AssetManager.h>
#include <Gfx/Ninepatch.h>
#include <UI/UITheme.h>

namespace UI {

void Drawable::preparePlaceholder(RenderBuffer* buffer)
{
    auto& renderer = the_renderer();
    style->value.visit(
        [](Empty) { },
        [&](Colour const&) {
            placeholder = appendDrawRectPlaceholder(buffer, renderer.shaderIds.untextured, false);
        },
        [&](DrawableStyle::Gradient const&) {
            placeholder = appendDrawRectPlaceholder(buffer, renderer.shaderIds.untextured, false);
        },
        [&](DrawableStyle::Ninepatch const& ninepatch) {
            placeholder = appendDrawNinepatchPlaceholder(buffer, dynamic_cast<Ninepatch&>(*ninepatch.ref.metadata().loaded_asset).texture, renderer.shaderIds.textured);
        },
        [&](DrawableStyle::Sprite const&) {
            placeholder = appendDrawRectPlaceholder(buffer, renderer.shaderIds.pixelArt, true);
        });
}

void Drawable::fillPlaceholder(Rect2I bounds)
{
    style->value.visit(
        [](Empty) { },
        [&](Colour const& colour) {
            placeholder.get<DrawRectPlaceholder>().fill(bounds, colour);
        },
        [&](DrawableStyle::Gradient const& gradient) {
            placeholder.get<DrawRectPlaceholder>().fill(bounds, gradient.color00, gradient.color01, gradient.color10, gradient.color11);
        },
        [&](DrawableStyle::Ninepatch const& ninepatch) {
            placeholder.get<DrawNinepatchPlaceholder>().fill(bounds, ninepatch.ref.get(), ninepatch.colour);
        },
        [&](DrawableStyle::Sprite const& sprite) {
            placeholder.get<DrawRectPlaceholder>().fill(bounds, sprite.ref.get(), sprite.colour);
        });
}

void Drawable::draw(RenderBuffer* buffer, Rect2I bounds)
{
    auto& renderer = the_renderer();
    style->value.visit(
        [](Empty) { },
        [&](Colour const& colour) {
            buffer->draw_single_rect(bounds, renderer.shaderIds.untextured, colour);
        },
        [&](DrawableStyle::Gradient const& gradient) {
            buffer->draw_single_rect(bounds, renderer.shaderIds.untextured, gradient.color00, gradient.color01, gradient.color10, gradient.color11);
        },
        [&](DrawableStyle::Ninepatch const& ninepatch) {
            buffer->draw_ninepatch(bounds, renderer.shaderIds.textured, dynamic_cast<Ninepatch&>(*ninepatch.ref.metadata().loaded_asset), ninepatch.colour);
        },
        [&](DrawableStyle::Sprite const& sprite) {
            buffer->draw_single_sprite(sprite.ref.get(), bounds, renderer.shaderIds.textured, sprite.colour);
        });
}

}
