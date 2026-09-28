/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Flags.h>
#include <Util/Memory.h>
#include <Util/Optional.h>
#include <Util/String.h>

class LineReader {
public:
    struct State {
        String current_line;
        smm current_line_number;
        smm start_of_next_line;
        bool at_end_of_file;
    };

    enum class Flags : u8 {
        SkipBlankLines,
        RemoveTrailingComments,
        COUNT,
    };
    static constexpr ::Flags DefaultFlags { Flags::SkipBlankLines, Flags::RemoveTrailingComments };

    LineReader(String filename, Blob data, ::Flags<Flags> flags = DefaultFlags, char commentChar = '#');

    State save_state() const;
    void restore_state(State const&);
    void restart();

    // FIXME: I don't like this API. Use a "has_next() / get_next()" style instead.
    //        Or, for_each_line() with a callback?
    //        Or, an iterator of some sort?
    bool load_next_line();
    StringView current_line() const;

    void warn(String message, std::initializer_list<StringView> args = {}) const;
    void error(String message, std::initializer_list<StringView> args = {}) const;
    Error make_error_message(String message, std::initializer_list<StringView> args = {}) const;

    u32 line_count() const;

private:
    String m_filename;
    Blob m_data;

    State m_state {};

    bool m_skip_blank_lines;
    bool m_remove_comments;
    char m_comment_char;
    mutable Optional<u32> m_line_count;
};
