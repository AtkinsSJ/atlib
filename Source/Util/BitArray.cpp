/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BitArray.h"
#include <Util/Assert.h>
#include <Util/MemoryArena.h>

BitArray::BitArray(MemoryArena& arena, u32 size)
    : m_size(size)
    , m_data(arena.allocate_filled_array<u64>(calculate_u64_count(size)))
{
}

BitArray BitArray::from_memory(u32 size, Array<u64> u64s)
{
    ASSERT(u64s.count() * 64 >= size);
    return BitArray { size, u64s };
}

BitArray::BitArray(u32 size, Array<u64> u64s)
    : m_size(size)
    , m_data(u64s)
{
}

bool BitArray::operator[](u32 index) const
{
    bool result = false;

    // NB: Check and assert done this way so that in debug builds, we assert, but
    // in release builds without asserts, we just return false for non-existent bits.
    if (index >= this->m_size || index < 0) {
        ASSERT(false);
    } else {
        u32 fieldIndex = index >> 6;
        u32 bitIndex = index & 63;
        u64 mask = ((u64)1 << bitIndex);
        result = (this->m_data[fieldIndex] & mask) != 0;
    }

    return result;
}

void BitArray::set_bit(u32 index)
{
    assign_bit(index, true);
}

void BitArray::unset_bit(u32 index)
{
    assign_bit(index, false);
}

void BitArray::assign_bit(u32 index, bool value)
{
    // NB: Check and assert done this way so that in debug builds, we assert, but
    // in release builds without asserts, we just do nothing for non-existent bits.
    if (index >= m_size || index < 0) {
        ASSERT(false);
    } else {
        u32 field_index = index >> 6;
        u32 bit_index = index & 63;
        u64 mask = ((u64)1 << bit_index);

        bool old_value = (m_data[field_index] & mask) != 0;

        if (old_value == value)
            return;
        if (value) {
            m_data[field_index] |= mask;
            m_set_bit_count++;
        } else {
            m_data[field_index] &= ~mask;
            m_set_bit_count--;
        }
    }
}

void BitArray::set_all()
{
    assign_all(true);
}

void BitArray::unset_all()
{
    assign_all(false);
}

void BitArray::assign_all(bool value)
{
    m_set_bit_count = value ? m_size : 0;

    // FIXME: memset?
    for (auto& it : m_data) {
        it = value ? ~0u : 0;
    }
}

Array<u32> BitArray::get_set_bit_indices() const
{
    Array<u32> result = temp_arena().allocate_array<u32>(m_set_bit_count);

    for (auto it = iterate_set_bits(); it.has_next(); it.next()) {
        result.append(it.get_index());
    }

    return result;
}

u32 BitArray::get_first_set_bit_index() const
{
    return get_first_matching_bit_index(true);
}

u32 BitArray::get_first_unset_bit_index() const
{
    return get_first_matching_bit_index(false);
}

u32 BitArray::get_first_matching_bit_index(bool set) const
{
    u32 result = -1;

    if (m_set_bit_count != m_size) {
        for (u32 index = 0; index < m_size; index++) {
            if ((*this)[index] == set) {
                result = index;
                break;
            }
        }
    }

    return result;
}

BitArrayIterator BitArray::iterate_set_bits() const
{
    return BitArrayIterator(*this);
}

//////////////////////////////////////////////////
// ITERATOR STUFF                               //
//////////////////////////////////////////////////

BitArrayIterator::BitArrayIterator(BitArray const& array)
    : m_array(array)
    , m_current_index(0)
    , m_is_done(array.is_all_unset())
{
    // If the first bit is unset, we need to skip ahead
    if (has_next() && !get_value())
        next();
}

void BitArrayIterator::next()
{
    while (!m_is_done) {
        m_current_index++;

        if (m_current_index >= m_array.size()) {
            m_is_done = true;
        } else {
            // Only stop iterating if we find a set bit
            if (get_value())
                break;
        }
    }
}

bool BitArrayIterator::has_next() const
{
    return !m_is_done;
}

u32 BitArrayIterator::get_index() const
{
    return m_current_index;
}

bool BitArrayIterator::get_value() const
{
    return m_array[m_current_index];
}
