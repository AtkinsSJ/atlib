/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Allocator.h"
#include <Util/Memory.h>
#include <Util/String.h>

String Allocator::allocate_string(StringView input)
{
    // We enforce null termination in the allocated string, because that makes things simpler.
    auto is_null_terminated = input.is_null_terminated();
    auto character_data = allocate_multiple<char>(input.length() + (is_null_terminated ? 0 : 1));
    copy_memory(input.raw_pointer_to_characters(), character_data.raw_data(), input.length());
    if (!is_null_terminated)
        character_data.raw_data()[input.length()] = 0;
    return String { character_data.raw_data(), input.length() };
}

Blob Allocator::allocate_blob(size_t size)
{
    return Blob { size, allocate_internal(size).raw_data() };
}

Blob Allocator::allocate_blob(Blob const& source)
{
    auto result = allocate_blob(source.size());
    copy_memory(source.data(), result.writable_data(), source.size());
    return result;
}

void Allocator::deallocate(Blob& blob)
{
    deallocate_internal({ blob.size(), blob.writable_data() });
    blob = {};
}

void Allocator::deallocate(String& string)
{
    string.~String();
    deallocate_internal({ string.length(), reinterpret_cast<u8*>(const_cast<char*>(string.raw_pointer_to_characters())) });
    string = {};
}
