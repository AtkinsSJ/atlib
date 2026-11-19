/*
 * Copyright (c) 2017-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Console.h"
#include <App/App.h>
#include <Assets/AssetManager.h>
#include <Debug/Keymap.h>
#include <Gfx/BitmapFont.h>
#include <Gfx/Renderer.h>
#include <IO/LineReader.h>
#include <Input/KeyboardShortcut.h>
#include <Settings/Settings.h>
#include <UI/Drawable.h>
#include <UI/TextInput.h>
#include <UI/Toast.h>
#include <Util/Lexer.h>

static Console* s_console = nullptr;

/*
 * Our custom logger posts the message into the built-in console, AND then sends it off to SDL's
 * usual logger. This way it's still useful even if our console is broken. (Which will happen!)
 */

static SDL_LogOutputFunction default_logger;
static void* default_logger_user_data;

void console_log_output_function(void* /*userdata*/, int category, SDL_LogPriority priority, char const* message)
{
    default_logger(default_logger_user_data, category, priority, message);

    ConsoleLineStyle style = ConsoleLineStyle::Default;

    switch (priority) {
    case SDL_LOG_PRIORITY_WARN:
        style = ConsoleLineStyle::Warning;
        break;

    case SDL_LOG_PRIORITY_ERROR:
    case SDL_LOG_PRIORITY_CRITICAL:
        style = ConsoleLineStyle::Error;
        break;

    default:
        style = ConsoleLineStyle::Default;
        break;
    }

    consoleWriteLine(String::from_null_terminated(message), style);
}

Console& Console::init(Allocator& allocator, float open_height, float maximised_height, float open_speed)
{
    s_console = new Console(allocator, open_height, maximised_height, open_speed);

    SDL_GetLogOutputFunction(&default_logger, &default_logger_user_data);
    SDL_SetLogOutputFunction(&console_log_output_function, nullptr);

    return *s_console;
}

Console::Console(Allocator& allocator, float open_height, float maximised_height, float open_speed)
    : m_current_height(0)
    , m_open_height(open_height)
    , m_maximised_height(maximised_height)
    , m_open_speed(open_speed)
    , m_input(allocator, consoleLineLength, "`"_s)
    , m_input_history(allocator, 256)
    , m_input_history_cursor(-1)
    , m_output_lines(allocator, 1024)
    , m_command_shortcuts(allocator, 128)
{
    register_default_commands();
}

Console& Console::the()
{
    ASSERT(s_console);
    return *s_console;
}

bool Console::is_enabled()
{
    return s_console != nullptr;
}

void Console::update_and_render(float delta_time)
{
    bool scrollToBottom = false;
    auto& renderer = the_renderer();

    if (!isInputCaptured()) {
        // Keyboard shortcuts for commands
        for (auto it = m_command_shortcuts.iterate();
            it.hasNext();
            it.next()) {
            auto& shortcut = it.get();

            if (shortcut.shortcut.was_just_pressed()) {
                handle_command(shortcut.command);
                scrollToBottom = true;
            }
        }
    }

    if (!isInputCaptured() || m_input.has_captured_input()) {
        // Show/hide the console
        if (keyJustPressed(SDLK_GRAVE)) {
            if (modifierKeyIsPressed(ModifierKey::Ctrl)) {
                if (m_target_height == m_maximised_height) {
                    m_target_height = m_open_height;
                } else {
                    m_target_height = m_maximised_height;
                }
                scrollToBottom = true;
            } else {
                if (m_target_height > 0) {
                    m_target_height = 0;
                } else {
                    m_target_height = m_open_height;
                    scrollToBottom = true;
                }
            }
        }
    }

    if (m_current_height != m_target_height) {
        m_current_height = approach(m_current_height, m_target_height, m_open_speed * delta_time);
    }

    // This is a little hacky... I think we want the console to ALWAYS consume input if it is open.
    // Other things can't take control from it.
    // Maybe this is a terrible idea? Not sure, but we'll see.
    // - Sam, 16/01/2020
    if (m_current_height > 0) {
        m_input.capture_input();
    } else {
        m_input.release_input();
    }

    if (m_input.has_captured_input()) {
        if (UI::updateTextInput(&m_input)) {
            handle_command(m_input.text());
            scrollToBottom = true;
            m_input.clear();
        } else {
            // Tab completion
            // FIXME: Move this behaviour into TextInput?
            if (keyJustPressed(SDLK_TAB)) {
                auto word_to_complete = m_input.last_word();
                if (!word_to_complete.is_empty()) {
                    // Search through our commands to find one that matches
                    // Eventually, we might want to cache an array of commands, but that's a low priority for now
                    Optional<StringView> command_to_complete;

                    for (auto& [name, command] : m_commands) {
                        if (name.starts_with(word_to_complete)) {
                            command_to_complete = name.view();
                            break;
                        }
                    }

                    if (command_to_complete.has_value())
                        m_input.insert(command_to_complete.value().substring(word_to_complete.length()));
                }
            }
            // Command history
            else if (keyJustPressed(SDLK_UP)) {
                if (m_input_history.count > 0) {
                    if (m_input_history_cursor == -1) {
                        m_input_history_cursor = truncate32(m_input_history.count - 1);
                    } else {
                        m_input_history_cursor = max(m_input_history_cursor - 1, 0);
                    }

                    m_input.clear();
                    String const& oldInput = m_input_history.get(m_input_history_cursor);
                    m_input.append(oldInput);
                }
            }
            // Command history
            else if (keyJustPressed(SDLK_DOWN)) {
                if (m_input_history.count > 0) {
                    if (m_input_history_cursor == -1) {
                        m_input_history_cursor = truncate32(m_input_history.count - 1);
                    } else {
                        m_input_history_cursor = truncate32(min(m_input_history_cursor + 1, (s32)m_input_history.count - 1));
                    }

                    m_input.clear();
                    String const& oldInput = m_input_history.get(m_input_history_cursor);
                    m_input.append(oldInput);
                }
            }
        }
    }

    // Display the console
    if (m_current_height > 0) {
        RenderBuffer* renderBuffer = &renderer.debug_buffer();
        s32 actualConsoleHeight = floor_s32(m_current_height * (float)renderer.window_height());
        s32 screenWidth = renderer.window_width();

        UI::ConsoleStyle& consoleStyle = m_style.get();
        UI::ScrollbarStyle& scrollbarStyle = consoleStyle.scrollbarStyle.get();
        UI::TextInputStyle& textInputStyle = consoleStyle.textInputStyle.get();

        // Text input
        V2I textInputSize = UI::calculateTextInputSize(&m_input, &textInputStyle, screenWidth);
        V2I textInputPos = v2i(0, actualConsoleHeight - textInputSize.y);
        Rect2I textInputBounds { textInputPos, textInputSize };
        UI::drawTextInput(renderBuffer, &m_input, &textInputStyle, textInputBounds);

        // Output area
        V2I textPos = v2i(consoleStyle.padding.left, textInputBounds.y());
        s32 textMaxWidth = screenWidth - (consoleStyle.padding.left + consoleStyle.padding.right);
        s32 heightOfOutputArea = textPos.y;
        Rect2I consoleBackRect { 0, 0, screenWidth, heightOfOutputArea };
        UI::Drawable(&consoleStyle.background).draw(renderBuffer, consoleBackRect);

        // Scrollbar
        Rect2I scrollbarBounds = get_scrollbar_bounds();
        if (scrollbarBounds.has_positive_area()) {
            s32 fontLineHeight = consoleStyle.font.get().line_height();
            s32 contentHeight = ((m_output_lines.count - 1) * fontLineHeight) + scrollbarBounds.height();
            if (scrollToBottom) {
                m_scrollbar.scroll_to_bottom();
            }

            m_scrollbar.set_mouse_wheel_step_size(3 * fontLineHeight);

            m_scrollbar.place(contentHeight, scrollbarBounds, &scrollbarStyle, false, renderBuffer);
        }

        textPos.y -= consoleStyle.padding.bottom;

        // print output lines
        auto& consoleFont = consoleStyle.font.get();
        s32 scrollLinePos = clamp(floor_s32(m_scrollbar.scroll_percent() * m_output_lines.count), 0, m_output_lines.count - 1);
        Alignment outputLinesAlign { HAlign::Left, VAlign::Bottom };
        for (auto it = m_output_lines.iterate(scrollLinePos, false, true);
            it.hasNext();
            it.next()) {
            auto& line = it.get();
            auto outputTextColor = consoleStyle.outputTextColors[line.style];

            V2I textSize = consoleFont.calculate_text_size(line.text, textMaxWidth);
            Rect2I textBounds = Rect2I::create_aligned(textPos, textSize, outputLinesAlign);
            drawText(renderBuffer, &consoleFont, line.text, textBounds, outputLinesAlign, outputTextColor, renderer.shaderIds.text);
            textPos.y -= (textSize.y + consoleStyle.contentPadding);

            // If we've gone off the screen, stop!
            if ((textPos.y < 0) || (textPos.y > renderer.window_height())) {
                break;
            }
        }
    }
}

void Console::handle_command(StringView command_input)
{
    // copy input to output, for readability
    write_line(myprintf("> {0}"_s, { command_input }), ConsoleLineStyle::InputEcho);

    if (!command_input.is_empty()) {
        // Add to history
        m_input_history.append(m_input_history.allocator->allocate_string(command_input));
        m_input_history_cursor = -1;

        Lexer lexer { command_input };
        if (auto command_token = lexer.consume_token(); command_token.has_value()) {
            lexer.discard_whitespace();

            // Find the command
            auto command = m_commands.get(command_token.value().deprecated_to_string());

            // Run the commands
            if (command.has_value()) {
                u32 commandStartTime = SDL_GetTicks();
                command->function(*this, lexer);
                u32 commandEndTime = SDL_GetTicks();

                write_line(myprintf("Command executed in {0}ms"_s, { formatInt(commandEndTime - commandStartTime) }));
            } else {
                write_line(myprintf("I don't understand '{0}'. Try 'help' for a list of commands."_s, { command_token.value() }), ConsoleLineStyle::Error);
            }
        }
    }
}

void Console::write_line(String text, ConsoleLineStyle line_style)
{
    ConsoleOutputLine* line = s_console->m_output_lines.appendBlank();
    line->style = line_style;
    line->text = s_console->m_output_lines.allocator->allocate_string(text);
}

void consoleWriteLine(String text, ConsoleLineStyle style)
{
    if (s_console)
        s_console->write_line(text, style);
}

Rect2I Console::get_scrollbar_bounds() const
{
    auto& renderer = the_renderer();
    UI::ConsoleStyle& consoleStyle = m_style.get();
    UI::ScrollbarStyle& scrollbarStyle = consoleStyle.scrollbarStyle.get();
    UI::TextInputStyle& textInputStyle = consoleStyle.textInputStyle.get();

    V2I textInputSize = UI::calculateTextInputSize(&m_input, &textInputStyle, renderer.window_width());

    Rect2I scrollbarBounds { static_cast<s32>(renderer.window_width() - scrollbarStyle.width), 0, scrollbarStyle.width, floor_s32(m_current_height * renderer.window_height()) - textInputSize.y };

    return scrollbarBounds;
}

void Console::register_command(Command&& command)
{
    auto name = command.name;
    m_commands.set(name, move(command));
}

void Console::after_assets_loaded()
{
    // Load the dev keymap
    auto const& keymap = Keymap::get("dev.keymap"_sv);
    for (auto const& shortcut : keymap.shortcuts())
        m_command_shortcuts.append(shortcut);
}

void Console::before_assets_unloaded()
{
    // Unload the dev keymap
    m_command_shortcuts.clear();
}

#define ConsoleCommand(name) static void cmd_##name([[maybe_unused]] Console& console, [[maybe_unused]] Lexer& arguments)
ConsoleCommand(exit)
{
    console.write_line("Quitting game..."_s, ConsoleLineStyle::Success);
    input_state().receivedQuitSignal = true;
}

ConsoleCommand(hello)
{
    console.write_line("Hello human!"_s);
    console.write_line(myprintf("Testing formatInt bases: 10:{0}, 16:{1}, 36:{2}, 8:{3}, 2:{4}"_s, { formatInt(123456, 10), formatInt(123456, 16), formatInt(123456, 36), formatInt(123456, 8), formatInt(123456, 2) }));
}

ConsoleCommand(help)
{
    console.write_line("Available commands are:"_s);

    for (auto& [name, command] : console.commands())
        console.write_line(myprintf(" - {0}"_s, { name }));
}

ConsoleCommand(reload_assets)
{
    asset_manager().reload();
}

ConsoleCommand(reload_settings)
{
    Settings::the().load();
    Settings::the().apply();
}

ConsoleCommand(setting)
{
    auto& settings = *Settings::the().settings;

    if (!arguments.has_next()) {
        console.write_line("Available settings:"_s, ConsoleLineStyle::Success);
        settings.for_each_setting([&](auto& setting) {
            console.write_line(setting.name(), ConsoleLineStyle::Success);
        });
        return;
    }

    auto maybe_setting_name = arguments.consume_token();
    if (!maybe_setting_name.has_value()) {
        console.write_line("Missing setting name."_s, ConsoleLineStyle::Error);
        return;
    }
    auto setting_name = maybe_setting_name.release_value();
    arguments.discard_whitespace();
    auto maybe_setting = settings.get_setting(setting_name.deprecated_to_string());
    if (!maybe_setting.has_value()) {
        console.write_line(myprintf("Unrecognized setting name '{}'."_s, { setting_name }), ConsoleLineStyle::Error);
        return;
    }
    auto& setting = *maybe_setting.release_value();

    if (!arguments.has_next()) {
        console.write_line(myprintf("{} is {}"_s, { setting_name, setting.serialize_value() }), ConsoleLineStyle::Success);
        return;
    }

    if (setting.read(arguments)) {
        Settings::the().apply();
        console.write_line(myprintf("Set {} to {}"_s, { setting_name, setting.serialize_value() }), ConsoleLineStyle::Success);
    } else {
        console.write_line(myprintf("Unable to set {}: invalid format"_s, { setting_name }), ConsoleLineStyle::Error);
    }
}

ConsoleCommand(speed)
{
    auto& app = App::the();

    if (!arguments.has_next()) {
        console.write_line(myprintf("Current game speed: {0}"_s, { formatFloat(app.speed_multiplier(), 3) }), ConsoleLineStyle::Success);
        return;
    }

    auto speed_multiplier = arguments.consume_float();
    arguments.discard_whitespace();
    if (speed_multiplier.has_value() && !arguments.has_next()) {
        float multiplier = speed_multiplier.value();
        app.set_speed_multiplier(multiplier);
        console.write_line(myprintf("Set speed to {0}"_s, { formatFloat(multiplier, 3) }), ConsoleLineStyle::Success);
        return;
    }

    console.write_line("Usage: `speed [MULTIPLIER]`, where MULTIPLIER is a float, or with no argument to list the current speed"_s, ConsoleLineStyle::Error);
}

ConsoleCommand(toast)
{
    auto text = arguments.consume_remainder();
    if (!text.has_value()) {
        console.write_line("Usage: `toast TEXT GOES HERE`"_s, ConsoleLineStyle::Error);
        return;
    }
    UI::Toast::show(text.release_value());
}

ConsoleCommand(zoom)
{
    auto& renderer = the_renderer();

    if (!arguments.has_next()) {
        // list the zoom
        float zoom = renderer.world_camera().zoom();
        console.write_line(myprintf("Current zoom is {0}"_s, { formatFloat(zoom, 3) }), ConsoleLineStyle::Success);
    } else {
        // set the zoom
        auto requested_zoom = arguments.consume_float();
        arguments.discard_whitespace();
        if (requested_zoom.has_value() && !arguments.has_next()) {
            float newZoom = requested_zoom.release_value();
            renderer.world_camera().set_zoom(newZoom);
            console.write_line(myprintf("Set zoom to {0}"_s, { formatFloat(newZoom, 3) }), ConsoleLineStyle::Success);
            return;
        }
        console.write_line("Usage: `zoom [SCALE]`, where SCALE is a float, or with no argument to list the current zoom"_s, ConsoleLineStyle::Error);
    }
}
#undef ConsoleCommand

void Console::register_default_commands()
{
    register_command(Command { "help"_s, cmd_help });
    register_command(Command { "exit"_s, cmd_exit });
    register_command(Command { "hello"_s, cmd_hello });
    register_command(Command { "reload_assets"_s, cmd_reload_assets });
    register_command(Command { "reload_settings"_s, cmd_reload_settings });
    register_command(Command { "setting"_s, cmd_setting });
    register_command(Command { "speed"_s, cmd_speed });
    register_command(Command { "toast"_s, cmd_toast });
    register_command(Command { "zoom"_s, cmd_zoom });
}
