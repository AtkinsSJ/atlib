/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Forward.h>
#include <Gfx/DrawRectsGroup.h>
#include <Gfx/DrawRingsGroup.h>
#include <Gfx/Placeholders.h>
#include <Gfx/RenderItem.h>
#include <Util/Pool.h>
#include <Util/String.h>

struct RenderBufferChunk final : public Poolable<RenderBufferChunk> {
    RenderBufferChunk() = default;

    // Poolable
    static RenderBufferChunk& allocate_from_pool(MemoryArena&);
    void initialize_from_pool() { }
    virtual void clear_for_pool() override;

    static constexpr u64 DEFAULT_SIZE = 64_KB;

    smm size;
    smm used;
    u8* memory;

    // TODO: @Size: Could potentially re-use the pointers from the PoolItem, if we wanted.
    RenderBufferChunk* prevChunk;
    RenderBufferChunk* nextChunk;
};

class RenderBufferReader {
public:
    RenderBufferReader(Badge<RenderBuffer>, RenderBufferChunk* start_chunk);

    bool is_eof() const;

    RenderItemType read_type();

    template<typename T>
    T const& read()
    {
        ASSERT(m_current_chunk);
        ASSERT(m_pos + sizeof(T) <= m_current_chunk->used);

        T* item = reinterpret_cast<T*>(m_current_chunk->memory + m_pos);
        m_pos += sizeof(T);
        return *item;
    }

    template<typename T>
    ReadonlySpan<T> read_multiple(size_t count)
    {
        ASSERT(m_current_chunk);
        ASSERT(m_pos + count * sizeof(T) <= m_current_chunk->used);

        T* item = reinterpret_cast<T*>(m_current_chunk->memory + m_pos);
        m_pos += sizeof(T) * count;
        return ReadonlySpan { count, item };
    }

    void next_chunk();

private:
    RenderBufferChunk* m_current_chunk;
    size_t m_pos;
};

class RenderBuffer final : public Poolable<RenderBuffer> {
public:
    friend class RenderBufferReader;

    RenderBuffer() = default;
    void take_from(RenderBuffer& other);

    bool is_empty() const;
    String const& name() const { return m_name; }

    void add_set_projection_matrix(Matrix4 const&);
    void add_set_shader(s8 shaderID);
    void add_clear(Colour clear_color = {});

    void add_set_texture(AssetMetadata& texture);
    void add_set_texture_raw(s32 width, s32 height, u8 bytes_per_pixel, ReadonlySpan<u8> pixels);
    void add_set_palette(ReadonlySpan<Colour> palette);

    // Scissors are a stack, so you can nest them. Note that nested scissors don't crop to each other!
    // NB: bounds rectangle is in window pixels, with the origin at the top-left
    void add_begin_scissor(Rect2I bounds);
    void add_end_scissor();

    void draw_single_sprite(Sprite const&, Rect2 bounds, s8 shader_id, Colour color);
    void draw_single_rect(Rect2 bounds, s8 shader_id, Colour color);
    void draw_single_rect(Rect2 bounds, s8 shader_id, Colour color00, Colour color01, Colour color10, Colour color11);
    void draw_ninepatch(Rect2I bounds, s8 shader_id, Ninepatch const&, Colour = Colour::white());
    void draw_grid(Rect2 bounds, Array2<u8> const& grid, ReadonlySpan<Colour> palette);

    DrawRectsGroup begin_rects_group_textured(AssetMetadata& texture, size_t capacity, Optional<s8> shader_id = {});
    DrawRectsGroup begin_rects_group_untextured(size_t capacity, Optional<s8> shader_id = {});
    DrawRectsGroup begin_rects_group_for_text(BitmapFont const& font, size_t capacity, Optional<s8> shader_id = {});

    DrawRingsGroup begin_rings_group(size_t capacity);

    // For when you want something to appear NOW in the render-order, but you don't know its details until later
    DrawRectPlaceholder append_draw_rect_placeholder(s8 shader_id, bool has_texture);
    DrawNinepatchPlaceholder append_draw_ninepatch_placeholder(AssetMetadata& texture, s8 shader_id);

    template<typename T>
    T& append_render_item(RenderItemType type)
    {
        u8* data = append_render_item_internal(type, sizeof(T), 0);
        return *new (data) T();
    }

    template<typename T>
    struct RenderItemAndData {
        T* item;
        Span<u8> data;
    };
    template<typename T>
    RenderItemAndData<T> append_render_item_with_data(RenderItemType type, size_t data_size)
    {
        RenderItemAndData<T> result = {};

        u8* buffer_data = append_render_item_internal(type, sizeof(T) + data_size, 0);
        result.item = new (buffer_data) T();
        result.data = { data_size, buffer_data + sizeof(T) };

        return result;
    }

    template<typename T>
    RenderItemAndData<T> append_render_item_with_reserved_data(RenderItemType type, size_t reserved_size)
    {
        ASSERT(!m_has_range_reserved);

        RenderItemAndData<T> result = {};

        u8* buffer_data = append_render_item_internal(type, sizeof(T), reserved_size);
        result.item = new (buffer_data) T();
        result.data = { reserved_size, buffer_data + sizeof(T) };

        m_has_range_reserved = true;
        return result;
    }

    void consume_reserved_memory(size_t size);

    RenderBufferReader read() const;

    // Poolable
    void initialize_from_pool(MemoryArena&, StringView source_name, Pool<RenderBufferChunk>*);
    virtual void clear_for_pool() override;

private:
    void append_render_item_type(RenderItemType);
    u8* append_render_item_internal(RenderItemType type, smm size, smm reserved_size);

    // NB: The Rects drawn must all have the same Texture!
    DrawRectsGroup begin_rects_group_internal(Optional<AssetMetadata&> texture, size_t max_count, s8 shader_id);

    String m_name;

    RenderBufferChunk* m_first_chunk;
    RenderBufferChunk* m_current_chunk;
    Pool<RenderBufferChunk>* m_chunk_pool;

    // Transient stuff
    bool m_has_range_reserved;
    s32 m_scissor_count;
    s8 m_current_shader;
    AssetMetadata* m_current_texture;
};
