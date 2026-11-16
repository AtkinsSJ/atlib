/*
 * Copyright (c) 2017-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Debug/Forward.h>
#include <Input/KeyboardShortcut.h>
#include <UI/TextInput.h>
#include <UI/UI.h>
#include <UI/UITheme.h>
#include <Util/ChunkedArray.h>
#include <Util/HashMap.h>

struct ConsoleOutputLine {
    String text;
    ConsoleLineStyle style;
};

struct CommandShortcut {
    KeyboardShortcut shortcut;
    StringView command;
};

using CommandFunction = void (*)(Console&, Lexer& arguments);
struct Command {
    String name;
    CommandFunction function;

    Command() = default;
    Command(String name, CommandFunction function)
    {
        this->name = name;
        this->function = function;
    }
};

class Console : public AssetManagerListener {
public:
    static Console& init(Allocator&, float open_height, float maximised_height, float open_speed);
    static Console& the();
    static bool is_enabled();

    virtual ~Console() override = default;

    void update_and_render(float delta_time);

    void write_line(String text, ConsoleLineStyle line_style = ConsoleLineStyle::Default);

    virtual void after_assets_loaded() override;
    virtual void before_assets_unloaded() override;

    void register_command(Command&&);
    void register_default_commands();
    HashMap<String, Command> const& commands() const { return m_commands; }

private:
    Console(Allocator&, float open_height, float maximised_height, float open_speed);

    void handle_command(StringView command_input);

    Rect2I get_scrollbar_bounds() const;

    TypedAssetRef<UI::ConsoleStyle> m_style { "default"_s };

    float m_current_height;
    float m_target_height;
    float m_open_height;      // % of screen height
    float m_maximised_height; // % of screen height
    float m_open_speed;       // % per second

    UI::TextInput m_input;
    ChunkedArray<String> m_input_history;
    s32 m_input_history_cursor;

    ChunkedArray<ConsoleOutputLine> m_output_lines;
    UI::Scrollbar m_scrollbar { Orientation::Vertical };

    HashMap<String, Command> m_commands { 128 };
    ChunkedArray<CommandShortcut> m_command_shortcuts;
};

s32 const consoleLineLength = 255;

void consoleWriteLine(String text, ConsoleLineStyle style = ConsoleLineStyle::Default);
