/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "LineReader.h"
#include <Util/Log.h>
#include <Util/Optional.h>
#include <Util/Unicode.h>

LineReader::LineReader(String filename, Blob data, ::Flags<Flags> flags, char commentChar)
    : m_filename(filename)
    , m_data(data)
    , m_skip_blank_lines(flags.has(Flags::SkipBlankLines))
    , m_remove_comments(flags.has(Flags::RemoveTrailingComments))
    , m_comment_char(commentChar)
{
}

LineReader::State LineReader::save_state() const
{
    return m_state;
}

void LineReader::restore_state(State const& position)
{
    m_state = position;
}

void LineReader::restart()
{
    m_state = {};
}

u32 LineReader::line_count() const
{
    if (!m_line_count.has_value()) {
        u32 result = 0;

        smm startOfNextLine = 0;

        // Code originally based on loadNextLine() but with a lot of alterations!
        do {
            ++result;
            while ((startOfNextLine < m_data.size()) && !is_newline(m_data.data()[startOfNextLine])) {
                ++startOfNextLine;
            }

            // Handle Windows' stupid double-character newline.
            if (startOfNextLine < m_data.size()) {
                ++startOfNextLine;
                if (is_newline(m_data.data()[startOfNextLine]) && (m_data.data()[startOfNextLine] != m_data.data()[startOfNextLine - 1])) {
                    ++startOfNextLine;
                }
            }
        } while (!(startOfNextLine >= m_data.size()));
        m_line_count = result;
    }

    return m_line_count.value();
}

bool LineReader::load_next_line()
{
    bool result = true;

    String line;

    do {
        // Get next line
        ++m_state.current_line_number;
        auto* line_chars = (char*)(m_data.data() + m_state.start_of_next_line);
        auto line_length = 0u;
        while ((m_state.start_of_next_line < m_data.size()) && !is_newline(m_data.data()[m_state.start_of_next_line])) {
            ++m_state.start_of_next_line;
            ++line_length;
        }

        // Handle Windows' stupid double-character newline.
        if (m_state.start_of_next_line < m_data.size()) {
            ++m_state.start_of_next_line;
            if (is_newline(m_data.data()[m_state.start_of_next_line]) && (m_data.data()[m_state.start_of_next_line] != m_data.data()[m_state.start_of_next_line - 1])) {
                ++m_state.start_of_next_line;
            }
        }

        // Trim the comment
        if (m_remove_comments) {
            for (s32 p = 0; p < line_length; p++) {
                if (line_chars[p] == m_comment_char) {
                    line_length = p;
                    break;
                }
            }
        }

        // Trim whitespace
        line = String { line_chars, line_length }.trimmed();

        // This seems weird, but basically: The break means all lines get returned if we're not skipping blank ones.
        if (!m_skip_blank_lines)
            break;
    } while (line.is_empty() && !(m_state.start_of_next_line >= m_data.size()));

    m_state.current_line = line;

    if (line.is_empty()) {
        if (m_skip_blank_lines) {
            result = false;
            m_state.at_end_of_file = true;
        } else if (m_state.start_of_next_line >= m_data.size()) {
            result = false;
            m_state.at_end_of_file = true;
        }
    }

    return result;
}

StringView LineReader::current_line() const
{
    return m_state.current_line;
}

void LineReader::warn(String message, std::initializer_list<StringView> args) const
{
    String text = myprintf(message, args, false);
    String lineNumber = m_state.at_end_of_file ? "EOF"_s : formatInt(m_state.current_line_number);
    logWarn("{0}:{1} - {2}"_s, { m_filename, lineNumber, text });
}

void LineReader::error(String message, std::initializer_list<StringView> args) const
{
    logError("{}"_s, { make_error_message(message, args) });
}

Error LineReader::make_error_message(String message, std::initializer_list<StringView> args) const
{
    String text = myprintf(message, args, false);
    String lineNumber = m_state.at_end_of_file ? "EOF"_s : formatInt(m_state.current_line_number);
    auto error = myprintf("{0}:{1} - {2}"_s, { m_filename, lineNumber, text });
    logError("{}"_s, { error });
    return error;
}
