/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetManagerListener.h>
#include <Assets/Forward.h>
#include <Gfx/Camera.h>
#include <Gfx/Colour.h>
#include <Gfx/RenderBuffer.h>
#include <Gfx/WindowResizeListener.h>
#include <SDL2/SDL_events.h>
#include <Settings/SettingsChangeListener.h>
#include <Util/Basic.h>
#include <Util/ChunkedArray.h>
#include <Util/MemoryArena.h>
#include <Util/Pool.h>
#include <Util/String.h>
#include <Util/Vector.h>

// General rendering code.

float const SECONDS_PER_FRAME = 1.0f / 60.0f;

class Renderer
    : public AssetManagerListener
    , public SettingsChangeListener {
public:
    virtual ~Renderer() override;

    static bool initialize(SDL_Window*);

    RenderBuffer* get_temporary_render_buffer(String name);
    void return_temporary_render_buffer(RenderBuffer&);

    MemoryArena& arena() { return m_arena; }

    SDL_Window* sdl_window() const { return m_sdl_window; }
    V2I window_size() const { return m_window_size; }
    void resize_window(s32 width, s32 height, bool fullscreen);
    u32 window_width() const { return m_window_size.x; }
    u32 window_height() const { return m_window_size.y; }
    bool window_is_fullscreen() const { return m_window_is_fullscreen; }

    Camera& world_camera() { return m_world_camera; }
    Camera& ui_camera() { return m_ui_camera; }

    RenderBuffer& world_buffer() { return *m_world_buffer; }
    RenderBuffer& world_overlay_buffer() { return *m_world_overlay_buffer; }
    RenderBuffer& ui_buffer() { return *m_ui_buffer; }
    RenderBuffer& window_buffer() { return *m_window_buffer; }
    RenderBuffer& debug_buffer() { return *m_debug_buffer; }

    // Not convinced this is the best way of doing it, but it's better than what we had before!
    // Really, we do want to have this stuff in code, because it's accessed a LOT and we don't
    // want to be doing a million hashtable lookups all the time. It does feel like a hack, but
    // practicality is more important than perfection!
    // - Sam, 23/07/2019
    struct
    {
        s8 paletted { 0 };
        s8 pixelArt { 0 };
        s8 text { 0 };
        s8 textured { 0 };
        s8 untextured { 0 };
    } shaderIds;

    virtual void on_window_resized(s32 width, s32 height) = 0;
    void register_window_resize_listener(WindowResizeListener&);
    void unregister_window_resize_listener(WindowResizeListener&);

    void render();
    virtual void render_internal() = 0;
    virtual void after_assets_loaded() override;
    virtual void before_assets_unloaded() override;
    virtual void free() = 0;

    void show_system_wait_cursor();
    void set_cursor(String name);
    void set_cursor_visible(bool);

    void handle_window_event(SDL_WindowEvent const&);

protected:
    explicit Renderer(SDL_Window*);

    // ^SettingsChangeListener
    virtual void on_settings_changed(Settings const&) override;

    MemoryArena m_arena {};

    Pool<RenderBuffer> m_render_buffer_pool;
    Pool<RenderBufferChunk> m_render_buffer_chunk_pool;

    Array<RenderBuffer*> m_render_buffers {};
    RenderBuffer* m_world_buffer { nullptr };
    RenderBuffer* m_world_overlay_buffer { nullptr };
    RenderBuffer* m_ui_buffer { nullptr };
    RenderBuffer* m_window_buffer { nullptr };
    RenderBuffer* m_debug_buffer { nullptr };

    SDL_Window* m_sdl_window { nullptr };
    V2I m_window_size {};
    bool m_window_is_fullscreen { false };

    Camera m_world_camera;
    Camera m_ui_camera;

    String m_current_cursor_name {};
    bool m_cursor_is_visible { true };
    SDL_Cursor* m_system_wait_cursor { nullptr };

    ChunkedArray<Ref<WindowResizeListener>> m_window_resize_listeners {};
};

Renderer& the_renderer();
void freeRenderer();
