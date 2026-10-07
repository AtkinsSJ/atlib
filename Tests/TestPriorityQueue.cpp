/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Harness/Harness.h"

#include <Util/PriorityQueue.h>
#include <Util/StringBuilder.h>

void test_main()
{
    PriorityQueue<char> alphabet;
    auto expect_string = [&alphabet](String expected) {
        StringBuilder builder;
        for (auto c : alphabet) {
            builder.append(c.item);
        }
        // printf("ALPHABET: `%s`, expected `%s`", builder.to_string_view().deprecated_to_string().raw_pointer_to_characters(), expected.raw_pointer_to_characters());
        EXPECT(builder.to_string_view() == expected);
    };

    alphabet.add('b', 2);
    expect_string("b"_s);
    alphabet.add('a', 1);
    expect_string("ab"_s);
    alphabet.add('f', 3);
    expect_string("abf"_s);
    alphabet.add('c', 2);
    expect_string("abcf"_s);
    alphabet.add('d', 2);
    expect_string("abcdf"_s);
    alphabet.add('e', 2);
    expect_string("abcdef"_s);

    EXPECT(alphabet.take_next() == 'a');
    expect_string("bcdef"_s);
    {
        auto [item, priority] = alphabet.take_next_with_priority();
        EXPECT(item == 'b');
        EXPECT(priority == 2);
    }
    expect_string("cdef"_s);
    EXPECT(alphabet.take_next() == 'c');
    expect_string("def"_s);
    EXPECT(alphabet.take_next() == 'd');
    expect_string("ef"_s);
    EXPECT(alphabet.take_next() == 'e');
    expect_string("f"_s);
    EXPECT(alphabet.take_next() == 'f');
    expect_string(""_s);
    EXPECT(alphabet.is_empty());
}
