/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Renderer.h"

#include <Assets/AssetManager.h>
#include <Debug/Debug.h>
#include <Gfx/BitmapFont.h>
#include <Gfx/Cursor.h>
#include <Gfx/DrawRectsGroup.h>
#include <Gfx/Ninepatch.h>
#include <Gfx/OpenGL.h>
#include <Settings/Settings.h>
#include <Util/Ref.h>

static Renderer* s_renderer;

float snapZoomLevel(float zoom)
{
    return (float)clamp(round_float(10 * zoom) * 0.1f, 0.1f, 10.0f);
}

Renderer::Renderer(SDL_Window* window)
    : m_arena("Renderer"_s)
    , m_render_buffer_pool(m_arena)
    , m_render_buffer_chunk_pool(m_arena)
    , m_sdl_window(window)
    , m_window_resize_listeners(m_arena, 32)
{
    SDL_GetWindowSize(window, &m_window_size.x, &m_window_size.y);

    m_world_buffer = &m_render_buffer_pool.obtain(m_arena, "WorldBuffer"_s, &m_render_buffer_chunk_pool);
    m_world_overlay_buffer = &m_render_buffer_pool.obtain(m_arena, "WorldOverlayBuffer"_s, &m_render_buffer_chunk_pool);
    m_ui_buffer = &m_render_buffer_pool.obtain(m_arena, "UIBuffer"_s, &m_render_buffer_chunk_pool);
    m_window_buffer = &m_render_buffer_pool.obtain(m_arena, "WindowBuffer"_s, &m_render_buffer_chunk_pool);
    m_debug_buffer = &m_render_buffer_pool.obtain(m_arena, "DebugBuffer"_s, &m_render_buffer_chunk_pool);

    m_render_buffers = m_arena.allocate_array<RenderBuffer*>(5);
    m_render_buffers.append(m_world_buffer);
    m_render_buffers.append(m_world_overlay_buffer);
    m_render_buffers.append(m_ui_buffer);
    m_render_buffers.append(m_window_buffer);
    m_render_buffers.append(m_debug_buffer);

    // Init cameras
    Rect2I screen_rect { 0, 0, m_window_size.x, m_window_size.y };
    m_world_camera = Camera(screen_rect, 10000.0f, -10000.0f);
    m_ui_camera = Camera(screen_rect, 10000.0f, -10000.0f, v2(screen_rect.size()) * 0.5f);

    // Hide cursor until stuff loads
    set_cursor_visible(false);
    m_arena.mark_reset_position();
}

Renderer::~Renderer()
{
    if (m_system_wait_cursor != nullptr) {
        SDL_FreeCursor(m_system_wait_cursor);
        m_system_wait_cursor = nullptr;
    }
    s_renderer = nullptr;
}

bool Renderer::initialize(SDL_Window* window)
{
    // TODO: Potentially support other renderers.
    auto* gl_renderer = new GL::Renderer(window);

    if (!gl_renderer->set_up_context()) {
        logCritical("Failed to create OpenGL context!"_s);
        delete gl_renderer;
        return false;
    }

    gl_renderer->m_arena.mark_reset_position();

    s_renderer = gl_renderer;
    Settings::the().register_listener(*s_renderer);
    return true;
}

Renderer& the_renderer()
{
    ASSERT(s_renderer);
    return *s_renderer;
}

void Renderer::handle_window_event(SDL_WindowEvent const& event)
{
    switch (event.event) {
    case SDL_WINDOWEVENT_SIZE_CHANGED: {
        m_window_size = v2i(event.data1, event.data2);

        on_window_resized(m_window_size.x, m_window_size.y);
        for (auto it = m_window_resize_listeners.iterate(); it.hasNext(); it.next()) {
            it.getValue()->on_window_resized(m_window_size.x, m_window_size.y);
        }

        ui_camera().set_position(v2(m_window_size) * 0.5f);
        ui_camera().set_screen_rect({ 0, 0, m_window_size.x, m_window_size.y });
    } break;
    }
}

void Renderer::register_window_resize_listener(WindowResizeListener& listener)
{
    m_window_resize_listeners.append(listener);
    listener.on_window_resized(window_width(), window_height());
}

void Renderer::unregister_window_resize_listener(WindowResizeListener& listener)
{
    m_window_resize_listeners.findAndRemove(Ref { listener });
}

void Renderer::render()
{
    DEBUG_POOL(&m_render_buffer_pool, "renderBufferPool");
    DEBUG_POOL(&m_render_buffer_chunk_pool, "renderChunkPool");

    render_internal();

    for (auto* buffer : m_render_buffers)
        buffer->clear_for_pool();
}

void Renderer::after_assets_loaded()
{
    if (!m_current_cursor_name.is_empty()) {
        set_cursor(m_current_cursor_name);
    }
}

void Renderer::before_assets_unloaded()
{
    show_system_wait_cursor();
}

void freeRenderer()
{
    s_renderer->free();
    delete s_renderer;
}

void Renderer::resize_window(s32 w, s32 h, bool fullscreen)
{
    SDL_RestoreWindow(m_sdl_window);

    // So, I'm not super sure how we want to handle fullscreen. Do we scan the
    // available resolution options and list them to select from? Do we just use
    // the native resolution? Do we use fullscreen or fullscreen-window?
    //
    // For now, I'm thinking we use the native resolution, as a fullscreen window.
    // - Sam, 20/05/2021

    // If the requested setup is what we already have, do nothing!
    // This avoids the window jumping about when you save the settings
    if ((w == s_renderer->window_width())
        && (h == s_renderer->window_height())
        && (fullscreen == s_renderer->window_is_fullscreen())) {
        return;
    }

    s32 newW = w;
    s32 newH = h;

    if (fullscreen) {
        // Fullscreen!
        // Grab the desktop resolution to use that
        s32 displayIndex = SDL_GetWindowDisplayIndex(m_sdl_window);
        SDL_DisplayMode displayMode;
        if (SDL_GetDesktopDisplayMode(displayIndex, &displayMode) == 0) {
            SDL_SetWindowDisplayMode(m_sdl_window, &displayMode);
            SDL_SetWindowFullscreen(m_sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            newW = displayMode.w;
            newH = displayMode.h;
        } else {
            logError("Failed to get desktop display mode: {0}"_s, { String::from_null_terminated(SDL_GetError()) });
        }
    } else {
        // Window!
        SDL_SetWindowSize(m_sdl_window, newW, newH);
        SDL_SetWindowFullscreen(m_sdl_window, 0);

        // Centre it
        s32 displayIndex = SDL_GetWindowDisplayIndex(m_sdl_window);
        SDL_Rect displayBounds;
        if (SDL_GetDisplayBounds(displayIndex, &displayBounds) == 0) {
            s32 leftBorder, rightBorder, topBorder, bottomBorder;
            SDL_GetWindowBordersSize(m_sdl_window, &topBorder, &leftBorder, &bottomBorder, &rightBorder);

            s32 windowW = w + leftBorder + rightBorder;
            s32 windowH = h + topBorder + bottomBorder;
            s32 windowLeft = (displayBounds.w - windowW) / 2;
            s32 windowTop = (displayBounds.h - windowH) / 2;

            SDL_SetWindowPosition(m_sdl_window, displayBounds.x + windowLeft, displayBounds.y + windowTop);
        } else {
            logError("Failed to get display bounds: {0}"_s, { String::from_null_terminated(SDL_GetError()) });

            // As a backup, just centre it on the main display
            SDL_SetWindowPosition(m_sdl_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
    }

    s_renderer->m_window_is_fullscreen = fullscreen;
}

void Renderer::show_system_wait_cursor()
{
    if (m_system_wait_cursor == nullptr) {
        m_system_wait_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
    }
    SDL_SetCursor(m_system_wait_cursor);
}

void Renderer::set_cursor(String name)
{
    DEBUG_FUNCTION();

    auto& new_cursor_asset = Cursor::get(name);
    m_current_cursor_name = name;
    new_cursor_asset.set_as_cursor();
}

void Renderer::set_cursor_visible(bool visible)
{
    m_cursor_is_visible = visible;
    SDL_ShowCursor(visible ? 1 : 0);
}

RenderBuffer* Renderer::get_temporary_render_buffer(String name)
{
    return &m_render_buffer_pool.obtain(m_arena, name, &m_render_buffer_chunk_pool);
}

void Renderer::return_temporary_render_buffer(RenderBuffer& buffer)
{
    m_render_buffer_pool.discard(buffer);
}

void Renderer::on_settings_changed(Settings const& settings)
{
    auto resolution = settings.get_typed_setting<V2I>("resolution"_s).value();
    auto windowed = settings.get_typed_setting<bool>("windowed"_s).value();
    resize_window(resolution.x, resolution.y, !windowed);
}

void appendRenderItemType(RenderBuffer* buffer, RenderItemType type)
{
    *(RenderItemType*)(buffer->currentChunk->memory + buffer->currentChunk->used) = type;
    buffer->currentChunk->used += sizeof(RenderItemType);
}

// NB: reservedSize is for extra data that you want to make sure there is room for,
// but you're not allocating it right away. Make sure not to do any other allocations
// in between, and make sure to allocate the space you used when you're done!
u8* appendRenderItemInternal(RenderBuffer* buffer, RenderItemType type, smm size, smm reservedSize)
{
    ASSERT(!buffer->hasRangeReserved); // Can't append renderitems while a range is reserved!

    smm totalSizeRequired = (smm)(sizeof(RenderItemType) + size + reservedSize + sizeof(RenderItemType));

    if (buffer->currentChunk == nullptr || ((buffer->currentChunk->size - buffer->currentChunk->used) <= totalSizeRequired)) {
        // Out of room! Push a "go to next chunk" item and append some more memory
        if (buffer->currentChunk != nullptr) {
            ASSERT((buffer->currentChunk->size - buffer->currentChunk->used) > sizeof(RenderItemType)); // Need space for the next-chunk message
            appendRenderItemType(buffer, RenderItemType::NextMemoryChunk);
        }

        RenderBufferChunk& new_chunk = buffer->chunkPool->obtain();
        new_chunk.used = 0;
        new_chunk.prevChunk = nullptr;
        new_chunk.nextChunk = nullptr;

        // Add to the renderbuffer
        if (buffer->currentChunk == nullptr) {
            buffer->firstChunk = &new_chunk;
            buffer->currentChunk = &new_chunk;
        } else {
            buffer->currentChunk->nextChunk = &new_chunk;
            new_chunk.prevChunk = buffer->currentChunk;
        }

        buffer->currentChunk = &new_chunk;
    }

    appendRenderItemType(buffer, type);
    u8* result = (buffer->currentChunk->memory + buffer->currentChunk->used);
    buffer->currentChunk->used += size;
    return result;
}

void addSetProjectionMatrix(RenderBuffer* buffer, Matrix4 const& matrix)
{
    buffer->add_set_projection_matrix(matrix);
}

void addSetShader(RenderBuffer* buffer, s8 shaderID)
{
    buffer->add_set_shader(shaderID);
}

void addSetTexture(RenderBuffer* buffer, AssetMetadata* texture)
{
    buffer->add_set_texture(*texture);
}

void addSetTextureRaw(RenderBuffer* buffer, s32 width, s32 height, u8 bytesPerPixel, u8* pixels)
{
    buffer->add_set_texture_raw(width, height, bytesPerPixel, { static_cast<size_t>(width * height * bytesPerPixel), pixels });
}

void addSetPalette(RenderBuffer* buffer, ReadonlySpan<Colour> palette)
{
    buffer->add_set_palette(palette);
}

void addClear(RenderBuffer* buffer, Colour clearColor)
{
    buffer->add_clear(clearColor);
}

void addBeginScissor(RenderBuffer* buffer, Rect2I bounds)
{
    buffer->add_begin_scissor(bounds);
}

void addEndScissor(RenderBuffer* buffer)
{
    buffer->add_end_scissor();
}

void drawSingleSprite(RenderBuffer* buffer, Sprite* sprite, Rect2 bounds, s8 shaderID, Colour color)
{
    buffer->draw_single_sprite(*sprite, bounds, shaderID, color);
}

void drawSingleRect(RenderBuffer* buffer, Rect2 bounds, s8 shaderID, Colour color)
{
    buffer->draw_single_rect(bounds, shaderID, color);
}

void drawSingleRect(RenderBuffer* buffer, Rect2 bounds, s8 shaderID, Colour color00, Colour color01, Colour color10, Colour color11)
{
    buffer->draw_single_rect(bounds, shaderID, color00, color01, color10, color11);
}

DrawRectPlaceholder appendDrawRectPlaceholder(RenderBuffer* buffer, s8 shaderID, bool hasTexture)
{
    return buffer->append_draw_rect_placeholder(shaderID, hasTexture);
}

void fillDrawRectPlaceholder(DrawRectPlaceholder* placeholder, Rect2 bounds, Colour color)
{
    placeholder->fill(bounds, color);
}

void fillDrawRectPlaceholder(DrawRectPlaceholder* placeholder, Rect2 bounds, Colour color00, Colour color01, Colour color10, Colour color11)
{
    placeholder->fill(bounds, color00, color01, color10, color11);
}

void fillDrawRectPlaceholder(DrawRectPlaceholder* placeholder, Rect2 bounds, Sprite* sprite, Colour color)
{
    placeholder->fill(bounds, *sprite, color);
}

void drawNinepatch(RenderBuffer* buffer, Rect2I bounds, s8 shaderID, Ninepatch* ninepatch, Colour color)
{
    buffer->draw_ninepatch(bounds, shaderID, *ninepatch, color);
}

DrawNinepatchPlaceholder appendDrawNinepatchPlaceholder(RenderBuffer* buffer, AssetMetadata* texture, s8 shaderID)
{
    return buffer->append_draw_ninepatch_placeholder(*texture, shaderID);
}

void fillDrawNinepatchPlaceholder(DrawNinepatchPlaceholder* placeholder, Rect2I bounds, Ninepatch* ninepatch, Colour color)
{
    placeholder->fill(bounds, *ninepatch, color);
}

DrawRectsGroup* beginRectsGroupInternal(RenderBuffer* buffer, AssetMetadata* texture, s8 shaderID, s32 maxCount)
{
    addSetShader(buffer, shaderID);
    if (texture != nullptr)
        addSetTexture(buffer, texture);

    DrawRectsGroup* result = temp_arena().allocate<DrawRectsGroup>();
    result->buffer = buffer;
    result->count = 0;
    result->maxCount = maxCount;
    result->texture = texture;

    result->firstSubGroup = beginRectsSubGroup(result);
    result->currentSubGroup = &result->firstSubGroup;

    return result;
}

DrawRectsGroup* beginRectsGroupTextured(RenderBuffer* buffer, AssetMetadata* texture, s8 shaderID, s32 maxCount)
{
    return &buffer->begin_rects_group_textured(*texture, maxCount, shaderID);
}

DrawRectsGroup* beginRectsGroupUntextured(RenderBuffer* buffer, s8 shaderID, s32 maxCount)
{
    return &buffer->begin_rects_group_untextured(maxCount, shaderID);
}

DrawRectsGroup* beginRectsGroupForText(RenderBuffer* buffer, BitmapFont* font, s8 shaderID, s32 maxCount)
{
    return &buffer->begin_rects_group_for_text(*font, maxCount, shaderID);
}

DrawRectsSubGroup beginRectsSubGroup(DrawRectsGroup* group)
{
    return group->begin_sub_group();
}

void endCurrentSubGroup(DrawRectsGroup* group)
{
    group->end_current_sub_group();
}

RenderItem_DrawRects_Item* reserve(DrawRectsGroup* group, s32 count)
{
    return group->reserve(count).raw_data();
}

void addUntexturedRect(DrawRectsGroup* group, Rect2 bounds, Colour color)
{
    group->add_untextured_rect(bounds, color);
}

void addGlyphRect(DrawRectsGroup* group, BitmapFontGlyph const* glyph, V2 position, Colour color)
{
    group->add_glyph_rect(*glyph, position, color);
}

void addSpriteRect(DrawRectsGroup* group, Sprite* sprite, Rect2 bounds, Colour color)
{
    group->add_sprite_rect(*sprite, bounds, color);
}

void offsetRange(DrawRectsGroup* group, s32 startIndex, s32 endIndexInclusive, float offsetX, float offsetY)
{
    group->offset_range(startIndex, endIndexInclusive, offsetX, offsetY);
}

void endRectsGroup(DrawRectsGroup* group)
{
    group->end();
}

void drawGrid(RenderBuffer* buffer, Rect2 bounds, Array2<u8> const& grid, ReadonlySpan<Colour> palette)
{
    buffer->draw_grid(bounds, grid, palette);
}

DrawRingsGroup* beginRingsGroup(RenderBuffer* buffer, s32 maxCount)
{
    return &buffer->begin_rings_group(maxCount);
}

DrawRingsSubGroup beginRingsSubGroup(DrawRingsGroup* group)
{
    return group->begin_sub_group();
}

void endCurrentSubGroup(DrawRingsGroup* group)
{
    group->end_current_sub_group();
}

void addRing(DrawRingsGroup* group, V2 centre, float radius, float thickness, Colour color)
{
    group->add_ring(centre, radius, thickness, color);
}

void endRingsGroup(DrawRingsGroup* group)
{
    group->end();
}
