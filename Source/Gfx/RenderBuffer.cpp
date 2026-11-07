/*
 * Copyright (c) 2025-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Gfx/RenderBuffer.h>

#include <Assets/AssetMetadata.h>
#include <Debug/Debug.h>
#include <Gfx/BitmapFont.h>
#include <Gfx/Ninepatch.h>
#include <Gfx/Renderer.h>
#include <Gfx/Sprite.h>

RenderBufferChunk& RenderBufferChunk::allocate_from_pool(MemoryArena& arena)
{
    auto [chunk, data] = arena.allocate_with_data<RenderBufferChunk>(DEFAULT_SIZE);
    chunk.size = DEFAULT_SIZE;
    chunk.used = 0;
    chunk.memory = data.raw_data();

    return chunk;
}

void RenderBufferChunk::clear_for_pool()
{
    used = 0;
    prevChunk = nullptr;
    nextChunk = nullptr;
}

void RenderBuffer::take_from(RenderBuffer& other)
{
    if (other.currentChunk != nullptr) {
        // If the target is empty, we can take a shortcut and just copy the pointers
        if (this->currentChunk == nullptr) {
            this->firstChunk = other.firstChunk;
            this->currentChunk = other.currentChunk;
        } else {
            ASSERT((this->currentChunk->size - this->currentChunk->used) > sizeof(RenderItemType)); // Need space for the next-chunk message
            append_render_item_type(RenderItemType::NextMemoryChunk);
            this->currentChunk->nextChunk = other.firstChunk;
            other.firstChunk->prevChunk = this->currentChunk;
            this->currentChunk = other.currentChunk;
        }
    }

    other.firstChunk = nullptr;
    other.currentChunk = nullptr;
    other.clear_for_pool(); // Make sure things are reset
}

void RenderBuffer::add_set_projection_matrix(Matrix4 const& matrix)
{
    auto& matrix_item = append_render_item<RenderItem_SetProjectionMatrix>(RenderItemType::SetProjectionMatrix);
    matrix_item.matrix = matrix;
}
void RenderBuffer::add_set_shader(s8 shader_id)
{
    if (currentShader != shader_id) {
        auto& shaderItem = append_render_item<RenderItem_SetShader>(RenderItemType::SetShader);
        shaderItem.shaderID = shader_id;

        currentShader = shader_id;
        // NB: Setting the opengl shader loses the texture, so we replicate that here.
        // I suppose in the future we could make sure to bind the texture when the shader changes, but
        // most of the time we want to set a new texture anyway, so that would be a waste of cpu time.
        currentTexture = nullptr;
    }
}
void RenderBuffer::add_set_texture(AssetMetadata& texture)
{
    ASSERT(texture.state == AssetMetadata::State::Loaded);

    if (currentTexture != &texture) {
        auto& textureItem = append_render_item<RenderItem_SetTexture>(RenderItemType::SetTexture);
        textureItem.texture = &texture;

        currentTexture = &texture;
    }
}

void RenderBuffer::add_set_texture_raw(s32 width, s32 height, u8 bytes_per_pixel, ReadonlySpan<u8> pixels)
{
    currentTexture = nullptr;

    smm pixel_data_size = (width * height * bytes_per_pixel);
    ASSERT(pixel_data_size == pixels.size());
    auto [item, data] = append_render_item_with_data<RenderItem_SetTexture>(RenderItemType::SetTexture, pixel_data_size);

    RenderItem_SetTexture* textureItem = item;
    textureItem->texture = nullptr;
    textureItem->width = width;
    textureItem->height = height;
    textureItem->bytesPerPixel = bytes_per_pixel;

    // FIXME: Spooky memcpy
    copyMemory(pixels.raw_data(), data.raw_data(), pixel_data_size);
}

void RenderBuffer::add_set_palette(ReadonlySpan<Colour> palette)
{
    auto [item, data] = append_render_item_with_data<RenderItem_SetPalette>(RenderItemType::SetPalette, sizeof(Colour) * palette.size());

    item->paletteSize = palette.size();

    // FIXME: Spooky memcpy
    copyMemory(palette.raw_data(), reinterpret_cast<Colour*>(data.raw_data()), palette.size());
}

void RenderBuffer::add_clear(Colour clear_colour)
{
    auto& clear = append_render_item<RenderItem_Clear>(RenderItemType::Clear);
    clear.clearColor = clear_colour;
}

void RenderBuffer::add_begin_scissor(Rect2I bounds)
{
    ASSERT(bounds.width() >= 0);
    ASSERT(bounds.height() >= 0);

    auto& scissor = append_render_item<RenderItem_BeginScissor>(RenderItemType::BeginScissor);

    // We have to flip the bounds rectangle vertically because OpenGL has the origin in the bottom-left,
    // whereas our system uses the top-left!

    auto& renderer = the_renderer();
    bounds.set_y(renderer.window_height() - bounds.y() - bounds.height());

    // Crop to window bounds
    scissor.bounds = bounds.intersected({ 0u, 0u, renderer.window_width(), renderer.window_height() });

    ASSERT(scissor.bounds.width() >= 0);
    ASSERT(scissor.bounds.height() >= 0);

    scissorCount++;
}

void RenderBuffer::add_end_scissor()
{
    ASSERT(scissorCount > 0);
    append_render_item<RenderItem_EndScissor>(RenderItemType::EndScissor);
    scissorCount--;
}

void RenderBuffer::draw_single_sprite(Sprite const& sprite, Rect2 bounds, s8 shader_id, Colour colour)
{
    add_set_shader(shader_id);
    add_set_texture(*sprite.texture);

    auto& rect = append_render_item<RenderItem_DrawSingleRect>(RenderItemType::DrawSingleRect);
    rect.bounds = bounds;
    rect.color00 = colour;
    rect.color01 = colour;
    rect.color10 = colour;
    rect.color11 = colour;
    rect.uv = sprite.uv;
}

void RenderBuffer::draw_single_rect(Rect2 bounds, s8 shader_id, Colour colour)
{
    draw_single_rect(bounds, shader_id, colour, colour, colour, colour);
}

void RenderBuffer::draw_single_rect(Rect2 bounds, s8 shader_id, Colour colour00, Colour colour01, Colour colour10, Colour colour11)
{
    add_set_shader(shader_id);

    auto& rect = append_render_item<RenderItem_DrawSingleRect>(RenderItemType::DrawSingleRect);
    rect.bounds = bounds;
    rect.color00 = colour00;
    rect.color01 = colour01;
    rect.color10 = colour10;
    rect.color11 = colour11;
    rect.uv = { 0.0f, 0.0f, 1.0f, 1.0f };
}

void RenderBuffer::draw_ninepatch(Rect2I bounds, s8 shader_id, Ninepatch const& ninepatch, Colour colour)
{
    auto& group = begin_rects_group_textured(*ninepatch.texture, 9, shader_id);

    // NB: See comments in the Ninepatch struct for how we could avoid doing the  UV calculations repeatedly,
    // and why we haven't done so.

    float x0 = static_cast<float>(bounds.x());
    float x1 = static_cast<float>(bounds.x() + ninepatch.pu1 - ninepatch.pu0);
    float x2 = static_cast<float>(bounds.x() + bounds.width() - (ninepatch.pu3 - ninepatch.pu2));
    float x3 = static_cast<float>(bounds.x() + bounds.width());

    float y0 = static_cast<float>(bounds.y());
    float y1 = static_cast<float>(bounds.y() + ninepatch.pv1 - ninepatch.pv0);
    float y2 = static_cast<float>(bounds.y() + bounds.height() - (ninepatch.pv3 - ninepatch.pv2));
    float y3 = static_cast<float>(bounds.y() + bounds.height());

    // top left
    group.add_rect(Rect2::create_min_max(x0, y0, x1, y1), colour, Rect2::create_min_max(ninepatch.u0, ninepatch.v0, ninepatch.u1, ninepatch.v1));

    // top
    group.add_rect(Rect2::create_min_max(x1, y0, x2, y1), colour, Rect2::create_min_max(ninepatch.u1, ninepatch.v0, ninepatch.u2, ninepatch.v1));

    // top-right
    group.add_rect(Rect2::create_min_max(x2, y0, x3, y1), colour, Rect2::create_min_max(ninepatch.u2, ninepatch.v0, ninepatch.u3, ninepatch.v1));

    // middle left
    group.add_rect(Rect2::create_min_max(x0, y1, x1, y2), colour, Rect2::create_min_max(ninepatch.u0, ninepatch.v1, ninepatch.u1, ninepatch.v2));

    // middle
    group.add_rect(Rect2::create_min_max(x1, y1, x2, y2), colour, Rect2::create_min_max(ninepatch.u1, ninepatch.v1, ninepatch.u2, ninepatch.v2));

    // middle-right
    group.add_rect(Rect2::create_min_max(x2, y1, x3, y2), colour, Rect2::create_min_max(ninepatch.u2, ninepatch.v1, ninepatch.u3, ninepatch.v2));

    // bottom left
    group.add_rect(Rect2::create_min_max(x0, y2, x1, y3), colour, Rect2::create_min_max(ninepatch.u0, ninepatch.v2, ninepatch.u1, ninepatch.v3));

    // bottom
    group.add_rect(Rect2::create_min_max(x1, y2, x2, y3), colour, Rect2::create_min_max(ninepatch.u1, ninepatch.v2, ninepatch.u2, ninepatch.v3));

    // bottom-right
    group.add_rect(Rect2::create_min_max(x2, y2, x3, y3), colour, Rect2::create_min_max(ninepatch.u2, ninepatch.v2, ninepatch.u3, ninepatch.v3));

    group.end();
}

void RenderBuffer::draw_grid(Rect2 bounds, Array2<u8> const& grid, ReadonlySpan<Colour> palette)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Renderer);

    add_set_shader(the_renderer().shaderIds.paletted);
    add_set_texture_raw(grid.width(), grid.height(), 1, grid.span());
    add_set_palette(palette);

    auto& rect = append_render_item<RenderItem_DrawSingleRect>(RenderItemType::DrawSingleRect);
    rect.bounds = bounds;
    rect.color00 = Colour::white();
    rect.color01 = Colour::white();
    rect.color10 = Colour::white();
    rect.color11 = Colour::white();
    rect.uv = { 0, 0, 1, 1 };
}

DrawRectPlaceholder RenderBuffer::append_draw_rect_placeholder(s8 shader_id, bool has_texture)
{
    add_set_shader(shader_id);

    DrawRectPlaceholder result = {};

    if (has_texture) {
        auto& textureItem = append_render_item<RenderItem_SetTexture>(RenderItemType::SetTexture);

        // We need to clear this, because we don't know what this texture will be, so any following draw calls
        // have to assume that they need to set their texture
        currentTexture = nullptr;

        result.set_texture_item({}, textureItem);
    }

    result.set_rect_item({}, append_render_item<RenderItem_DrawSingleRect>(RenderItemType::DrawSingleRect));

    return result;
}

DrawNinepatchPlaceholder RenderBuffer::append_draw_ninepatch_placeholder(AssetMetadata& texture, s8 shader_id)
{
    auto& rectsGroup = begin_rects_group_textured(texture, 9, shader_id);
    DrawNinepatchPlaceholder placeholder { rectsGroup.reserve(9) };
    rectsGroup.end();

    return placeholder;
}

void RenderBuffer::initialize_from_pool(MemoryArena& arena, StringView source_name, Pool<RenderBufferChunk>* chunk_pool)
{
    this->name = arena.allocate_string(source_name);
    this->name.hash();

    this->hasRangeReserved = false;
    this->scissorCount = 0;

    this->chunkPool = chunk_pool;

    this->firstChunk = nullptr;
    this->currentChunk = nullptr;

    this->currentShader = -1;
    this->currentTexture = nullptr;
}

void RenderBuffer::clear_for_pool()
{
    currentShader = -1;
    currentTexture = nullptr;

    for (RenderBufferChunk* chunk = firstChunk; chunk != nullptr;) {
        // NB: `discard()` wipes the nextChunk member, so we need to grab it first.
        auto* next_chunk = chunk->nextChunk;
        chunkPool->discard(*chunk);
        chunk = next_chunk;
    }

    firstChunk = nullptr;
    currentChunk = nullptr;
}
void RenderBuffer::append_render_item_type(RenderItemType type)
{
    *reinterpret_cast<RenderItemType*>(currentChunk->memory + currentChunk->used) = type;
    currentChunk->used += sizeof(RenderItemType);
}

// NB: reservedSize is for extra data that you want to make sure there is room for,
// but you're not allocating it right away. Make sure not to do any other allocations
// in between, and make sure to allocate the space you used when you're done!
u8* RenderBuffer::append_render_item_internal(RenderItemType type, smm size, smm reserved_size)
{
    ASSERT(!hasRangeReserved); // Can't append renderitems while a range is reserved!

    smm totalSizeRequired = static_cast<smm>(sizeof(RenderItemType) + size + reserved_size + sizeof(RenderItemType));

    if (currentChunk == nullptr || ((currentChunk->size - currentChunk->used) <= totalSizeRequired)) {
        // Out of room! Push a "go to next chunk" item and append some more memory
        if (currentChunk != nullptr) {
            ASSERT((currentChunk->size - currentChunk->used) > sizeof(RenderItemType)); // Need space for the next-chunk message
            append_render_item_type(RenderItemType::NextMemoryChunk);
        }

        RenderBufferChunk& new_chunk = chunkPool->obtain();
        new_chunk.used = 0;
        new_chunk.prevChunk = nullptr;
        new_chunk.nextChunk = nullptr;

        // Add to the renderbuffer
        if (currentChunk == nullptr) {
            firstChunk = &new_chunk;
            currentChunk = &new_chunk;
        } else {
            currentChunk->nextChunk = &new_chunk;
            new_chunk.prevChunk = currentChunk;
        }

        currentChunk = &new_chunk;
    }

    append_render_item_type(type);
    u8* result = (currentChunk->memory + currentChunk->used);
    currentChunk->used += size;
    return result;
}

DrawRectsGroup& RenderBuffer::begin_rects_group_internal(Optional<AssetMetadata&> texture, s32 max_count, s8 shader_id)
{
    add_set_shader(shader_id);
    if (texture.has_value())
        add_set_texture(texture.value());

    DrawRectsGroup* result = temp_arena().allocate<DrawRectsGroup>();
    result->buffer = this;
    result->count = 0;
    result->maxCount = max_count;
    if (texture.has_value())
        result->texture = &texture.value();
    else
        result->texture = nullptr;

    result->firstSubGroup = result->begin_sub_group();
    result->currentSubGroup = &result->firstSubGroup;

    return *result;
}

DrawRectsGroup& RenderBuffer::begin_rects_group_textured(AssetMetadata& texture, s32 max_count, Optional<s8> shader_id)
{
    return begin_rects_group_internal(texture, max_count, shader_id.value_or(the_renderer().shaderIds.textured));
}

DrawRectsGroup& RenderBuffer::begin_rects_group_untextured(s32 max_count, Optional<s8> shader_id)
{
    return begin_rects_group_internal({}, max_count, shader_id.value_or(the_renderer().shaderIds.untextured));
}

DrawRectsGroup& RenderBuffer::begin_rects_group_for_text(BitmapFont const& font, s32 max_count, Optional<s8> shader_id)
{
    return begin_rects_group_internal(*font.texture(), max_count, shader_id.value_or(the_renderer().shaderIds.text));
}

DrawRingsGroup& RenderBuffer::begin_rings_group(s32 max_count)
{
    // @Copypasta begin_rects_group_internal() - maybe factor out common "grouped render items" code?
    add_set_shader(the_renderer().shaderIds.untextured);

    DrawRingsGroup* result = temp_arena().allocate<DrawRingsGroup>();
    result->buffer = this;
    result->count = 0;
    result->maxCount = max_count;

    result->firstSubGroup = beginRingsSubGroup(result);
    result->currentSubGroup = &result->firstSubGroup;

    return *result;
}
