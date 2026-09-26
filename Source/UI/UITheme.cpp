/*
 * Copyright (c) 2017-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Assets/AssetManager.h>
#include <Assets/ContainerAsset.h>
#include <Gfx/BitmapFont.h>
#include <IO/LineReader.h>
#include <UI/UITheme.h>
#include <Util/Lexer.h>

namespace UI {

static HashMap<String, Property> s_style_properties { 256 };
static HashMap<String, StyleType> s_style_types_by_name { 256 };

ErrorOr<DrawableStyle> DrawableStyle::read(Lexer& lexer)
{
    return lexer.consume_with_callback_or_error<DrawableStyle>([](Lexer& lexer) -> ErrorOr<DrawableStyle> {
        lexer.discard_whitespace();
        auto maybe_type_name = lexer.consume_token();
        if (!maybe_type_name.has_value())
            return "Missing drawable type name"_s;
        auto type_name = maybe_type_name.release_value();
        lexer.discard_whitespace();

        if (type_name == "none"_s)
            return DrawableStyle {};

        if (type_name == "color"_s) {
            auto colour = Colour::read(lexer);
            lexer.discard_whitespace();
            if (colour.is_error())
                return colour.release_error();
            return DrawableStyle { colour.release_value() };
        }

        if (type_name == "gradient"_s) {
            auto color00 = Colour::read(lexer);
            lexer.discard_whitespace();
            auto color01 = Colour::read(lexer);
            lexer.discard_whitespace();
            auto color10 = Colour::read(lexer);
            lexer.discard_whitespace();
            auto color11 = Colour::read(lexer);
            lexer.discard_whitespace();

            if (color00.is_error() || color01.is_error() || color10.is_error() || color11.is_error())
                return "`gradient` drawable requires 4 colours"_s;
            return DrawableStyle { DrawableStyle::Gradient {
                .color00 = color00.release_value(),
                .color01 = color01.release_value(),
                .color10 = color10.release_value(),
                .color11 = color11.release_value(),
            } };
        }

        if (type_name == "ninepatch"_s) {
            auto ninepatch_name = lexer.consume_token();
            if (!ninepatch_name.has_value())
                return "Missing name for `ninepatch` drawable"_s;
            lexer.discard_whitespace();

            auto maybe_colour = Colour::read(lexer);
            lexer.discard_whitespace();
            auto colour = Colour::white();
            if (!maybe_colour.is_error())
                colour = maybe_colour.release_value();

            return DrawableStyle { DrawableStyle::Ninepatch {
                .ref = TypedAssetRef<::Ninepatch> { asset_manager().assetStrings.intern(ninepatch_name.value()) },
                .colour = colour,
            } };
        }

        if (type_name == "sprite"_s) {
            auto sprite_name = lexer.consume_token();
            if (!sprite_name.has_value())
                return "Missing name for `sprite` drawable"_s;
            lexer.discard_whitespace();

            auto maybe_colour = Colour::read(lexer);
            lexer.discard_whitespace();
            auto colour = Colour::white();
            if (!maybe_colour.is_error())
                colour = maybe_colour.release_value();

            return DrawableStyle { DrawableStyle::Sprite {
                .ref = SpriteRef { asset_manager().assetStrings.intern(sprite_name.value()), 0 },
                .colour = colour,
            } };
        }

        return "Unrecognized drawable type"_s;
    });
}

bool DrawableStyle::has_fixed_size() const
{
    return value.visit(
        [](Empty) { return true; },
        [&](DrawableStyle::Sprite const&) { return true; },
        [&](auto const&) { return false; });
}

V2I DrawableStyle::get_size() const
{
    return value.visit(
        [&](DrawableStyle::Sprite const& sprite) {
            auto& the_sprite = sprite.ref.get();
            return v2i(the_sprite.pixelWidth, the_sprite.pixelHeight);
        },
        [&](auto const&) { return v2i(0, 0); });
}

bool DrawableStyle::is_visible() const
{
    // TODO: Maybe return false if the opacity is 0 too?
    return !value.has<Empty>();
}

ButtonStyle::ButtonStyle(TypedAssetRef<BitmapFont> font, Colour text_color, Alignment text_alignment, Padding padding, s32 content_padding, DrawableStyle start_icon, Alignment start_icon_alignment, DrawableStyle end_icon, Alignment end_icon_alignment, DrawableStyle background, DrawableStyle background_hover, DrawableStyle background_pressed, DrawableStyle background_disabled)
    : font(move(font))
    , textColor(move(text_color))
    , textAlignment(move(text_alignment))
    , padding(move(padding))
    , contentPadding(move(content_padding))
    , startIcon(move(start_icon))
    , startIconAlignment(move(start_icon_alignment))
    , endIcon(move(end_icon))
    , endIconAlignment(move(end_icon_alignment))
    , background(move(background))
    , backgroundHover(move(background_hover))
    , backgroundPressed(move(background_pressed))
    , backgroundDisabled(move(background_disabled))
{
}

CheckboxStyle::CheckboxStyle(Padding padding, DrawableStyle background, DrawableStyle background_hover, DrawableStyle background_pressed, DrawableStyle background_disabled, V2I check_size, DrawableStyle check, DrawableStyle check_hover, DrawableStyle check_pressed, DrawableStyle check_disabled)
    : padding(move(padding))
    , background(move(background))
    , backgroundHover(move(background_hover))
    , backgroundPressed(move(background_pressed))
    , backgroundDisabled(move(background_disabled))
    , checkSize(move(check_size))
    , check(move(check))
    , checkHover(move(check_hover))
    , checkPressed(move(check_pressed))
    , checkDisabled(move(check_disabled))
{
}

ConsoleStyle::ConsoleStyle(TypedAssetRef<BitmapFont> font, EnumMap<ConsoleLineStyle, Colour> output_text_colors, DrawableStyle background, Padding padding, s32 content_padding, TypedAssetRef<ScrollbarStyle> scrollbar_style, TypedAssetRef<TextInputStyle> text_input_style)
    : font(move(font))
    , outputTextColors(move(output_text_colors))
    , background(move(background))
    , padding(move(padding))
    , contentPadding(move(content_padding))
    , scrollbarStyle(move(scrollbar_style))
    , textInputStyle(move(text_input_style))
{
}

DropDownListStyle::DropDownListStyle(TypedAssetRef<ButtonStyle> button_style, TypedAssetRef<PanelStyle> panel_style)
    : buttonStyle(move(button_style))
    , panelStyle(move(panel_style))
{
}

LabelStyle::LabelStyle(Padding padding, DrawableStyle background, TypedAssetRef<BitmapFont> font, Colour text_color, Alignment text_alignment)
    : padding(move(padding))
    , background(move(background))
    , font(move(font))
    , textColor(move(text_color))
    , textAlignment(move(text_alignment))
{
}

PanelStyle::PanelStyle(Padding padding, s32 content_padding, Alignment widget_alignment, DrawableStyle background, TypedAssetRef<ButtonStyle> button_style,
    TypedAssetRef<CheckboxStyle> checkbox_style,
    TypedAssetRef<DropDownListStyle> drop_down_list_style,
    TypedAssetRef<LabelStyle> label_style,
    TypedAssetRef<RadioButtonStyle> radio_button_style,
    TypedAssetRef<ScrollbarStyle> scrollbar_style,
    TypedAssetRef<SliderStyle> slider_style,
    TypedAssetRef<TextInputStyle> text_input_style)
    : padding(move(padding))
    , contentPadding(move(content_padding))
    , widgetAlignment(move(widget_alignment))
    , background(move(background))
    , buttonStyle(move(button_style))
    , checkboxStyle(move(checkbox_style))
    , dropDownListStyle(move(drop_down_list_style))
    , labelStyle(move(label_style))
    , radioButtonStyle(move(radio_button_style))
    , scrollbarStyle(move(scrollbar_style))
    , sliderStyle(move(slider_style))
    , textInputStyle(move(text_input_style))
{
}

RadioButtonStyle::RadioButtonStyle(V2I size, DrawableStyle background, DrawableStyle background_hover, DrawableStyle background_pressed, DrawableStyle background_disabled, V2I dot_size, DrawableStyle dot, DrawableStyle dot_hover, DrawableStyle dot_pressed, DrawableStyle dot_disabled)
    : size(move(size))
    , background(move(background))
    , backgroundHover(move(background_hover))
    , backgroundPressed(move(background_pressed))
    , backgroundDisabled(move(background_disabled))
    , dotSize(move(dot_size))
    , dot(move(dot))
    , dotHover(move(dot_hover))
    , dotPressed(move(dot_pressed))
    , dotDisabled(move(dot_disabled))
{
}

ScrollbarStyle::ScrollbarStyle(s32 width, DrawableStyle background, DrawableStyle thumb, DrawableStyle thumb_hover, DrawableStyle thumb_pressed, DrawableStyle thumb_disabled)
    : width(move(width))
    , background(move(background))
    , thumb(move(thumb))
    , thumbHover(move(thumb_hover))
    , thumbPressed(move(thumb_pressed))
    , thumbDisabled(move(thumb_disabled))
{
}

SliderStyle::SliderStyle(DrawableStyle track, s32 track_thickness, DrawableStyle thumb, DrawableStyle thumb_hover, DrawableStyle thumb_pressed, DrawableStyle thumb_disabled, V2I thumb_size)
    : track(move(track))
    , trackThickness(move(track_thickness))
    , thumb(move(thumb))
    , thumbHover(move(thumb_hover))
    , thumbPressed(move(thumb_pressed))
    , thumbDisabled(move(thumb_disabled))
    , thumbSize(move(thumb_size))
{
}

TextInputStyle::TextInputStyle(TypedAssetRef<BitmapFont> font, Colour text_color, Alignment text_alignment, DrawableStyle background, Padding padding, bool show_caret, float caret_flash_cycle_duration)
    : font(move(font))
    , textColor(move(text_color))
    , textAlignment(move(text_alignment))
    , background(move(background))
    , padding(move(padding))
    , showCaret(show_caret)
    , caretFlashCycleDuration(caret_flash_cycle_duration)
{
}

WindowStyle::WindowStyle(TypedAssetRef<LabelStyle> title_label_style, s32 title_bar_height, Colour title_bar_color, Colour title_bar_color_inactive, Colour title_bar_button_hover_color, V2I offset_from_mouse, TypedAssetRef<PanelStyle> panel_style)
    : titleLabelStyle(move(title_label_style))
    , titleBarHeight(move(title_bar_height))
    , titleBarColor(move(title_bar_color))
    , titleBarColorInactive(move(title_bar_color_inactive))
    , titleBarButtonHoverColor(move(title_bar_button_hover_color))
    , offsetFromMouse(move(offset_from_mouse))
    , panelStyle(move(panel_style))
{
}

void Style::set_property(StringView property, PropertyValue&& value)
{
    m_properties.set(property.deprecated_to_string(), move(value));
}

bool Style::get_bool(StringView property, bool default_value) const
{
    return get_property_value<bool>(property).value_or(default_value);
}

float Style::get_float(StringView property, float default_value) const
{
    return get_property_value<float>(property).value_or(default_value);
}

s32 Style::get_s32(StringView property, s32 default_value) const
{
    return get_property_value<s32>(property).value_or(default_value);
}

Alignment Style::get_alignment(StringView property, Alignment const& default_value) const
{
    return get_property_value<Alignment>(property).value_or(default_value);
}

Colour Style::get_colour(StringView property, Colour const& default_value) const
{
    return get_property_value<Colour>(property).value_or(default_value);
}

DrawableStyle Style::get_drawable_style(StringView property, DrawableStyle const& default_value) const
{
    return get_property_value<DrawableStyle>(property).value_or(default_value);
}

Padding Style::get_padding(StringView property, Padding const& default_value) const
{
    return get_property_value<Padding>(property).value_or(default_value);
}

String Style::get_string(StringView property, String const& default_value) const
{
    return get_property_value<String>(property).value_or(default_value);
}

V2I Style::get_v2i(StringView property, V2I const& default_value) const
{
    return get_v2i(property).value_or(default_value);
}

Optional<V2I> Style::get_v2i(StringView property) const
{
    return get_property_value<V2I>(property);
}

void initStyleConstants()
{
#define PROP(name, _type) s_style_properties.set(#name##_s, Property { .type = PropType::_type });

    PROP(background, Drawable);
    PROP(backgroundDisabled, Drawable);
    PROP(backgroundHover, Drawable);
    PROP(backgroundPressed, Drawable);
    PROP(buttonStyle, Style);
    PROP(caretFlashCycleDuration, Float);
    PROP(check, Drawable);
    PROP(checkDisabled, Drawable);
    PROP(checkHover, Drawable);
    PROP(checkPressed, Drawable);
    PROP(checkboxStyle, Style);
    PROP(checkSize, IntSize);
    PROP(dot, Drawable);
    PROP(dotDisabled, Drawable);
    PROP(dotHover, Drawable);
    PROP(dotPressed, Drawable);
    PROP(dotSize, IntSize);
    PROP(contentPadding, Int);
    PROP(dropDownListStyle, Style);
    PROP(endIcon, Drawable);
    PROP(endIconAlignment, Alignment);
    PROP(font, Font);
    PROP(labelStyle, Style);
    PROP(offsetFromMouse, IntPosition);
    PROP(outputTextColor, Color);
    PROP(outputTextColorInputEcho, Color);
    PROP(outputTextColorError, Color);
    PROP(outputTextColorWarning, Color);
    PROP(outputTextColorSuccess, Color);
    PROP(padding, Padding);
    PROP(panelStyle, Style);
    PROP(radioButtonStyle, Style);
    PROP(scrollbarStyle, Style);
    PROP(showCaret, Bool);
    PROP(size, IntSize);
    PROP(sliderStyle, Style);
    PROP(startIcon, Drawable);
    PROP(startIconAlignment, Alignment);
    PROP(textAlignment, Alignment);
    PROP(textColor, Color);
    PROP(textInputStyle, Style);
    PROP(thumb, Drawable);
    PROP(thumbDisabled, Drawable);
    PROP(thumbHover, Drawable);
    PROP(thumbPressed, Drawable);
    PROP(titleBarButtonHoverColor, Color);
    PROP(titleBarColor, Color);
    PROP(titleBarColorInactive, Color);
    PROP(titleBarHeight, Int);
    PROP(titleLabelStyle, Style);
    PROP(trackThickness, Int);
    PROP(widgetAlignment, Alignment);
    PROP(width, Int);
    PROP(track, Drawable);
    PROP(thumbSize, IntSize);

#undef PROP

    assignStyleProperties(StyleType::Button, {
                                                 "background"_s,
                                                 "backgroundDisabled"_s,
                                                 "backgroundHover"_s,
                                                 "backgroundPressed"_s,
                                                 "contentPadding"_s,
                                                 "endIcon"_s,
                                                 "endIconAlignment"_s,
                                                 "font"_s,
                                                 "padding"_s,
                                                 "startIcon"_s,
                                                 "startIconAlignment"_s,
                                                 "textAlignment"_s,
                                                 "textColor"_s,
                                             });
    assignStyleProperties(StyleType::Checkbox, {
                                                   "background"_s,
                                                   "backgroundDisabled"_s,
                                                   "backgroundHover"_s,
                                                   "backgroundPressed"_s,
                                                   "check"_s,
                                                   "checkDisabled"_s,
                                                   "checkHover"_s,
                                                   "checkSize"_s,
                                                   "checkPressed"_s,
                                                   "dropDownListStyle"_s,
                                                   "padding"_s,
                                               });
    assignStyleProperties(StyleType::Console, {
                                                  "background"_s,
                                                  "font"_s,
                                                  "outputTextColor"_s,
                                                  "outputTextColorInputEcho"_s,
                                                  "outputTextColorError"_s,
                                                  "outputTextColorSuccess"_s,
                                                  "outputTextColorWarning"_s,
                                                  "padding"_s,
                                                  "contentPadding"_s,
                                                  "scrollbarStyle"_s,
                                                  "textInputStyle"_s,
                                              });
    assignStyleProperties(StyleType::DropDownList, {
                                                       "buttonStyle"_s,
                                                       "panelStyle"_s,
                                                   });
    assignStyleProperties(StyleType::Label, {
                                                "background"_s,
                                                "font"_s,
                                                "padding"_s,
                                                "textColor"_s,
                                                "textAlignment"_s,
                                            });
    assignStyleProperties(StyleType::Panel, {
                                                "background"_s,
                                                "buttonStyle"_s,
                                                "checkboxStyle"_s,
                                                "contentPadding"_s,
                                                "dropDownListStyle"_s,
                                                "labelStyle"_s,
                                                "padding"_s,
                                                "radioButtonStyle"_s,
                                                "scrollbarStyle"_s,
                                                "sliderStyle"_s,
                                                "textInputStyle"_s,
                                                "widgetAlignment"_s,
                                            });
    assignStyleProperties(StyleType::RadioButton, {
                                                      "background"_s,
                                                      "backgroundDisabled"_s,
                                                      "backgroundHover"_s,
                                                      "backgroundPressed"_s,
                                                      "dot"_s,
                                                      "dotDisabled"_s,
                                                      "dotHover"_s,
                                                      "dotPressed"_s,
                                                      "dotSize"_s,
                                                      "size"_s,
                                                  });
    assignStyleProperties(StyleType::Scrollbar, {
                                                    "background"_s,
                                                    "thumb"_s,
                                                    "thumbDisabled"_s,
                                                    "thumbHover"_s,
                                                    "thumbPressed"_s,
                                                    "width"_s,
                                                });
    assignStyleProperties(StyleType::Slider, {
                                                 "track"_s,
                                                 "trackThickness"_s,
                                                 "thumb"_s,
                                                 "thumbDisabled"_s,
                                                 "thumbHover"_s,
                                                 "thumbPressed"_s,
                                                 "thumbSize"_s,
                                             });
    assignStyleProperties(StyleType::TextInput, {
                                                    "background"_s,
                                                    "caretFlashCycleDuration"_s,
                                                    "font"_s,
                                                    "padding"_s,
                                                    "showCaret"_s,
                                                    "textAlignment"_s,
                                                    "textColor"_s,
                                                });
    assignStyleProperties(StyleType::Window, {
                                                 "offsetFromMouse"_s,
                                                 "panelStyle"_s,
                                                 "titleBarButtonHoverColor"_s,
                                                 "titleBarColor"_s,
                                                 "titleBarColorInactive"_s,
                                                 "titleBarHeight"_s,
                                                 "titleLabelStyle"_s,
                                             });

    s_style_types_by_name.set("Button"_s, StyleType::Button);
    s_style_types_by_name.set("Checkbox"_s, StyleType::Checkbox);
    s_style_types_by_name.set("Console"_s, StyleType::Console);
    s_style_types_by_name.set("DropDownList"_s, StyleType::DropDownList);
    s_style_types_by_name.set("Label"_s, StyleType::Label);
    s_style_types_by_name.set("Panel"_s, StyleType::Panel);
    s_style_types_by_name.set("RadioButton"_s, StyleType::RadioButton);
    s_style_types_by_name.set("Scrollbar"_s, StyleType::Scrollbar);
    s_style_types_by_name.set("Slider"_s, StyleType::Slider);
    s_style_types_by_name.set("TextInput"_s, StyleType::TextInput);
    s_style_types_by_name.set("Window"_s, StyleType::Window);
}

void assignStyleProperties(StyleType type, std::initializer_list<String> properties)
{
    for (String* propName = (String*)properties.begin(); propName < properties.end(); propName++) {
        auto& property = s_style_properties.get(*propName).value();
        property.existsInStyle[type] = true;
    }
}

ErrorOr<OwnedRef<Asset>> load_theme(AssetMetadata& metadata, Blob data)
{
    LineReader reader { metadata.shortName, data };

    HashMap<String, EnumMap<StyleType, Style>> styles;

    HashMap<String, String> fontNamesToAssetNames;

    EnumMap<StyleType, s32> style_count;

    StringView currentSection;
    Style* target = nullptr;

    while (reader.load_next_line()) {
        Lexer lexer { reader.current_line() };

        // Commands
        if (lexer.consume_specific(':')) {
            // Define something
            auto maybe_command = lexer.consume_token();
            if (!maybe_command.has_value())
                return reader.make_error_message("Invalid command"_s);
            auto command = maybe_command.release_value();
            lexer.discard_whitespace();

            if (command == "Font"_s) {
                target = nullptr;
                auto font_name = lexer.consume_token();
                lexer.discard_whitespace();
                auto font_filename = lexer.consume_token();
                lexer.discard_whitespace();

                if (!font_name.has_value() || !font_filename.has_value() || lexer.has_next())
                    return reader.make_error_message("Invalid font command: Expected `:Font NAME PATH`"_s);

                AssetMetadata* font_asset = asset_manager().add_asset(BitmapFont::asset_type(), font_filename.value());
                fontNamesToAssetNames.set(font_name.value().deprecated_to_string(), font_asset->shortName);

            } else {
                // Create a new style entry if the name matches a style type
                if (auto found_style_type = s_style_types_by_name.get(command.deprecated_to_string()); found_style_type.has_value()) {
                    auto style_type = found_style_type.release_value();
                    auto name_token = lexer.consume_token();
                    lexer.discard_whitespace();
                    if (!name_token.has_value() || lexer.has_next())
                        return reader.make_error_message("Invalid `{0}` command: Expected `:{0} NAME`"_s, { command });

                    String name = asset_manager().assetStrings.intern(name_token.value());

                    auto& pack = styles.ensure(name, [] { return EnumMap<StyleType, Style> {}; });
                    target = &pack[style_type];
                    target->name = name;
                    target->type = style_type;

                    style_count[style_type]++;
                } else {
                    return reader.make_error_message("Unrecognized command: '{0}'"_s, { command });
                }
            }
            continue;
        }

        // Properties of the item
        // These are arranged alphabetically
        auto maybe_property = lexer.consume_token();
        if (!maybe_property.has_value())
            continue;
        auto property_name = maybe_property.release_value();
        lexer.discard_whitespace();

        if (property_name == "extends"_s) {
            // Clones an existing style
            auto parent_style_token = lexer.consume_token();
            lexer.discard_whitespace();
            if (!parent_style_token.has_value())
                return reader.make_error_message("Missing style name for `extends`"_s);
            if (lexer.has_next())
                return reader.make_error_message("Unexpected trailing text"_s);

            auto parent_style = parent_style_token.release_value().deprecated_to_string();
            auto parent_pack = styles.get(parent_style);
            if (!parent_pack.has_value())
                return reader.make_error_message("Unable to find style named '{0}'"_s, { parent_style });

            Style const& parent = parent_pack.value()[target->type];
            // For undefined styles, the parent struct will be all nulls, so the type will not match
            if (parent.type != target->type)
                return reader.make_error_message("Attempting to extend a style of the wrong type."_s);

            String name = target->name;
            *target = parent;
            target->name = name;
            continue;
        }

        // Check our properties map for a match
        if (auto property = s_style_properties.get(property_name.deprecated_to_string()); property.has_value()) {
            if (property->existsInStyle[target->type]) {
                switch (property->type) {
                case PropType::Alignment: {
                    auto value = Alignment::read(lexer);
                    lexer.discard_whitespace();
                    if (value.is_error())
                        return reader.make_error_message("Couldn't parse value of {0}: {1}"_s, { property_name, value.release_error() });
                    if (lexer.has_next())
                        return reader.make_error_message("Couldn't parse value of {0}: Expected an alignment"_s, { property_name });

                    target->set_property(property_name, value.release_value());
                } break;

                case PropType::Bool: {
                    auto value = lexer.consume_bool();
                    lexer.discard_whitespace();
                    if (!value.has_value() || lexer.has_next())
                        return reader.make_error_message("Couldn't parse value of {0}: Expected a boolean"_s, { property_name });

                    target->set_property(property_name, value.release_value());
                } break;

                case PropType::Color: {
                    auto value = Colour::read(lexer);
                    lexer.discard_whitespace();
                    if (value.is_error())
                        return reader.make_error_message("Couldn't parse value of {0}: {1}"_s, { property_name, value.release_error() });
                    if (lexer.has_next())
                        return reader.make_error_message("Couldn't parse value of {0}: Expected a colour"_s, { property_name });

                    target->set_property(property_name, value.release_value());
                } break;

                case PropType::Drawable: {
                    auto value = DrawableStyle::read(lexer);
                    lexer.discard_whitespace();
                    if (value.is_error())
                        return reader.make_error_message("Couldn't parse value of {0}: {1}"_s, { property_name, value.release_error() });
                    if (lexer.has_next())
                        return reader.make_error_message("Couldn't parse value of {0}: Expected a drawable"_s, { property_name });

                    target->set_property(property_name, value.release_value());
                } break;

                case PropType::Float: {
                    auto value = lexer.consume_float<float>();
                    lexer.discard_whitespace();
                    if (!value.has_value() || lexer.has_next())
                        return reader.make_error_message("Couldn't parse value of {0}: Expected a number"_s, { property_name });

                    target->set_property(property_name, value.release_value());
                } break;

                case PropType::Font: {
                    auto name_token = lexer.consume_token();
                    lexer.discard_whitespace();
                    if (!name_token.has_value() || lexer.has_next())
                        return reader.make_error_message("Couldn't parse value of {0}: Expected a font name"_s, { property_name });

                    String value = asset_manager().assetStrings.intern(name_token.value());
                    if (auto fontFilename = fontNamesToAssetNames.get(value); fontFilename.has_value()) {
                        target->set_property(property_name, fontFilename.value());
                    } else {
                        return reader.make_error_message("Unrecognised font name '{0}'. Make sure to declare the :Font before it is used!"_s, { value });
                    }
                } break;

                case PropType::Int: {
                    auto value = lexer.consume_int<s32>();
                    lexer.discard_whitespace();
                    if (!value.has_value() || lexer.has_next())
                        return reader.make_error_message("Couldn't parse value of {0}: Expected an integer"_s, { property_name });

                    target->set_property(property_name, value.release_value());
                } break;

                case PropType::IntPosition: {
                    auto value = V2I::read_position(lexer);
                    lexer.discard_whitespace();
                    if (!value.has_value() || lexer.has_next())
                        return reader.make_error_message("Couldn't parse value of {0}: Expected an integer position"_s, { property_name });

                    target->set_property(property_name, value.release_value());
                } break;

                case PropType::IntSize: {
                    auto value = V2I::read_size(lexer);
                    lexer.discard_whitespace();
                    if (!value.has_value() || lexer.has_next())
                        return reader.make_error_message("Couldn't parse value of {0}: Expected an integer size"_s, { property_name });

                    target->set_property(property_name, value.release_value());
                } break;

                case PropType::Padding: {
                    auto value = Padding::read(lexer);
                    lexer.discard_whitespace();
                    if (!value.has_value() || lexer.has_next())
                        return reader.make_error_message("Couldn't parse value of {0}: Expected a padding value (1 to 4 integers)"_s, { property_name });

                    target->set_property(property_name, value.release_value());
                } break;

                case PropType::Style: {
                    auto name_token = lexer.consume_token();
                    lexer.discard_whitespace();
                    if (!name_token.has_value() || lexer.has_next())
                        return reader.make_error_message("Couldn't parse value of {0}: Expected a style name"_s, { property_name });

                    String value = asset_manager().assetStrings.intern(name_token.value());
                    // Strings are read directly, so we don't need an if(valid) check
                    target->set_property(property_name, value);
                } break;

                default:
                    logCritical("Invalid property type for '{0}'"_s, { property_name });
                }
            } else {
                return reader.make_error_message("Property '{0}' is not allowed in '{1}'"_s, { property_name, currentSection });
            }
        } else {
            return reader.make_error_message("Unrecognized property '{0}'"_s, { property_name });
        }
    }

    // Actually write out the styles into the UITheme

    size_t total_style_count = 0;
    for (auto style_type : enum_values<StyleType>()) {
        total_style_count += style_count[style_type];
    }
    auto children = asset_manager().allocate_array<GenericAssetRef>(total_style_count);

    // Some default values to use
    auto transparent = Colour::from_rgb_255(0, 0, 0, 0);
    auto white = Colour::white();
    auto default_font_name = ""_h;

    for (auto& [key, style_pack] : styles) {
        for (auto style_type : enum_values<StyleType>()) {
            if (style_type == StyleType::None)
                continue;

            Style const* style = &(style_pack)[style_type];
            // For undefined styles, the parent struct will be all nulls, so the type will not match
            // FIXME: This seems really sketchy.
            if (style->type == style_type) {
                switch (style->type) {
                case StyleType::Button: {
                    auto font = style->get_asset_ref<BitmapFont>("font"_h, default_font_name);
                    auto text_color = style->get_colour("textColor"_h, white);
                    auto text_alignment = style->get_alignment("textAlignment"_h, { HAlign::Left, VAlign::Top });

                    auto padding = style->get_padding("padding"_h, {});
                    auto content_padding = style->get_s32("contentPadding"_h, {});

                    auto start_icon = style->get_drawable_style("startIcon"_h, {});
                    auto start_icon_alignment = style->get_alignment("startIconAlignment"_h, { HAlign::Left, VAlign::Top });
                    auto end_icon = style->get_drawable_style("endIcon"_h, {});
                    auto end_icon_alignment = style->get_alignment("endIconAlignment"_h, { HAlign::Right, VAlign::Top });

                    auto background = style->get_drawable_style("background"_h, {});
                    auto background_hover = style->get_drawable_style("backgroundHover"_h, background);
                    auto background_pressed = style->get_drawable_style("backgroundPressed"_h, background);
                    auto background_disabled = style->get_drawable_style("backgroundDisabled"_h, background);

                    if (!start_icon.has_fixed_size())
                        return reader.make_error_message("Start icon for button '{0}' has no fixed size. Defaulting to 0 x 0"_s, { style->name });

                    if (!end_icon.has_fixed_size())
                        return reader.make_error_message("End icon for button '{0}' has no fixed size. Defaulting to 0 x 0"_s, { style->name });

                    AssetMetadata* child_metadata = asset_manager().add_asset(ButtonStyle::asset_type(), style->name, {});
                    child_metadata->loaded_asset = adopt_own(*new ButtonStyle(
                        move(font), move(text_color), move(text_alignment), move(padding), move(content_padding),
                        move(start_icon), move(start_icon_alignment), move(end_icon), move(end_icon_alignment),
                        move(background), move(background_hover), move(background_pressed), move(background_disabled)));
                    child_metadata->state = AssetMetadata::State::Loaded;
                    children.append(child_metadata->get_ref());
                } break;

                case StyleType::Checkbox: {
                    auto padding = style->get_padding("padding"_h, {});

                    auto background = style->get_drawable_style("background"_h, {});
                    auto background_hover = style->get_drawable_style("backgroundHover"_h, background);
                    auto background_pressed = style->get_drawable_style("backgroundPressed"_h, background);
                    auto background_disabled = style->get_drawable_style("backgroundDisabled"_h, background);

                    auto check = style->get_drawable_style("check"_h, {});
                    auto check_hover = style->get_drawable_style("checkHover"_h, check);
                    auto check_pressed = style->get_drawable_style("checkPressed"_h, check);
                    auto check_disabled = style->get_drawable_style("checkDisabled"_h, check);

                    // Default check_size to the check image's size if it has one
                    V2I check_size {};
                    if (auto read_size = style->get_v2i("checkSize"_h); read_size.has_value()) {
                        check_size = read_size.release_value();
                    } else if (check.has_fixed_size()) {
                        check_size = check.get_size();
                    } else {
                        return reader.make_error_message("Check for checkbox '{0}' has no fixed size, and no checkSize was provided. Defaulting to 0 x 0"_s, { style->name });
                    }

                    AssetMetadata* child_metadata = asset_manager().add_asset(CheckboxStyle::asset_type(), style->name, {});
                    child_metadata->loaded_asset = adopt_own(*new CheckboxStyle(
                        move(padding),
                        move(background), move(background_hover), move(background_pressed), move(background_disabled),
                        move(check_size), move(check), move(check_hover), move(check_pressed), move(check_disabled)));
                    child_metadata->state = AssetMetadata::State::Loaded;
                    children.append(child_metadata->get_ref());
                } break;

                case StyleType::Console: {
                    auto font = style->get_asset_ref<BitmapFont>("font"_h, default_font_name);

                    auto default_color = style->get_colour("outputTextColor"_h, white);
                    EnumMap<ConsoleLineStyle, Colour> output_text_colors;
                    output_text_colors[ConsoleLineStyle::Default] = default_color;
                    output_text_colors[ConsoleLineStyle::InputEcho] = style->get_colour("outputTextColorInputEcho"_h, default_color);
                    output_text_colors[ConsoleLineStyle::Error] = style->get_colour("outputTextColorError"_h, default_color);
                    output_text_colors[ConsoleLineStyle::Warning] = style->get_colour("outputTextColorWarning"_h, default_color);
                    output_text_colors[ConsoleLineStyle::Success] = style->get_colour("outputTextColorSuccess"_h, default_color);

                    auto background = style->get_drawable_style("background"_h, {});
                    auto padding = style->get_padding("padding"_h, {});
                    auto content_padding = style->get_s32("contentPadding"_h, 0);

                    auto scrollbar_style = style->get_asset_ref<ScrollbarStyle>("scrollbarStyle"_h);
                    auto text_input_style = style->get_asset_ref<TextInputStyle>("textInputStyle"_h);

                    AssetMetadata* child_metadata = asset_manager().add_asset(ConsoleStyle::asset_type(), style->name, {});
                    child_metadata->loaded_asset = adopt_own(*new ConsoleStyle(
                        move(font), move(output_text_colors), move(background), move(padding), move(content_padding), move(scrollbar_style), move(text_input_style)));
                    child_metadata->state = AssetMetadata::State::Loaded;
                    children.append(child_metadata->get_ref());
                } break;

                case StyleType::DropDownList: {
                    auto button_style = style->get_asset_ref<ButtonStyle>("buttonStyle"_h);
                    auto panel_style = style->get_asset_ref<PanelStyle>("panelStyle"_h);

                    AssetMetadata* child_metadata = asset_manager().add_asset(DropDownListStyle::asset_type(), style->name, {});
                    child_metadata->loaded_asset = adopt_own(*new DropDownListStyle(move(button_style), move(panel_style)));
                    child_metadata->state = AssetMetadata::State::Loaded;
                    children.append(child_metadata->get_ref());
                } break;

                case StyleType::Label: {
                    auto padding = style->get_padding("padding"_h, {});
                    auto background = style->get_drawable_style("background"_h, {});
                    auto font = style->get_asset_ref<BitmapFont>("font"_h, default_font_name);
                    auto text_color = style->get_colour("textColor"_h, white);
                    auto text_alignment = style->get_alignment("textAlignment"_h, { HAlign::Left, VAlign::Top });

                    AssetMetadata* child_metadata = asset_manager().add_asset(LabelStyle::asset_type(), style->name, {});
                    child_metadata->loaded_asset = adopt_own(*new LabelStyle(move(padding), move(background), move(font), move(text_color), move(text_alignment)));
                    child_metadata->state = AssetMetadata::State::Loaded;
                    children.append(child_metadata->get_ref());
                } break;

                case StyleType::Panel: {
                    auto padding = style->get_padding("padding"_h, {});
                    auto content_padding = style->get_s32("contentPadding"_h, {});
                    auto widget_alignment = style->get_alignment("widgetAlignment"_h, { HAlign::Fill, VAlign::Top });
                    auto background = style->get_drawable_style("background"_h, {});

                    auto button_style = style->get_asset_ref<ButtonStyle>("buttonStyle"_h);
                    auto checkbox_style = style->get_asset_ref<CheckboxStyle>("checkboxStyle"_h);
                    auto drop_down_list_style = style->get_asset_ref<DropDownListStyle>("dropDownListStyle"_h);
                    auto label_style = style->get_asset_ref<LabelStyle>("labelStyle"_h);
                    auto radio_button_style = style->get_asset_ref<RadioButtonStyle>("radioButtonStyle"_h);
                    auto scrollbar_style = style->get_asset_ref<ScrollbarStyle>("scrollbarStyle"_h);
                    auto slider_style = style->get_asset_ref<SliderStyle>("sliderStyle"_h);
                    auto text_input_style = style->get_asset_ref<TextInputStyle>("textInputStyle"_h);

                    AssetMetadata* child_metadata = asset_manager().add_asset(PanelStyle::asset_type(), style->name, {});
                    child_metadata->loaded_asset = adopt_own(*new PanelStyle(
                        move(padding), move(content_padding), move(widget_alignment), move(background),
                        move(button_style), move(checkbox_style), move(drop_down_list_style), move(label_style), move(radio_button_style), move(scrollbar_style), move(slider_style), move(text_input_style)));
                    child_metadata->state = AssetMetadata::State::Loaded;
                    children.append(child_metadata->get_ref());
                } break;

                case StyleType::RadioButton: {
                    auto size = style->get_v2i("size"_h, {});
                    auto background = style->get_drawable_style("background"_h, {});
                    auto background_hover = style->get_drawable_style("backgroundDisabled"_h, background);
                    auto background_pressed = style->get_drawable_style("backgroundHover"_h, background);
                    auto background_disabled = style->get_drawable_style("backgroundPressed"_h, background);
                    auto dot_size = style->get_v2i("dotSize"_h, {});
                    auto dot = style->get_drawable_style("dot"_h, {});
                    auto dot_hover = style->get_drawable_style("dotDisabled"_h, dot);
                    auto dot_pressed = style->get_drawable_style("dotHover"_h, dot);
                    auto dot_disabled = style->get_drawable_style("dotPressed"_h, dot);

                    AssetMetadata* child_metadata = asset_manager().add_asset(RadioButtonStyle::asset_type(), style->name, {});
                    child_metadata->loaded_asset = adopt_own(*new RadioButtonStyle(
                        move(size),
                        move(background), move(background_hover), move(background_pressed), move(background_disabled),
                        move(dot_size), move(dot), move(dot_hover), move(dot_pressed), move(dot_disabled)));
                    child_metadata->state = AssetMetadata::State::Loaded;
                    children.append(child_metadata->get_ref());
                } break;

                case StyleType::Scrollbar: {
                    auto width = style->get_s32("width"_h, 8);
                    auto background = style->get_drawable_style("background"_h, {});
                    auto thumb = style->get_drawable_style("thumb"_h, {});
                    auto thumb_hover = style->get_drawable_style("thumbHover"_h, thumb);
                    auto thumb_pressed = style->get_drawable_style("thumbPressed"_h, thumb);
                    auto thumb_disabled = style->get_drawable_style("thumbDisabled"_h, thumb);

                    AssetMetadata* child_metadata = asset_manager().add_asset(ScrollbarStyle::asset_type(), style->name, {});
                    child_metadata->loaded_asset = adopt_own(*new ScrollbarStyle(
                        width, move(background),
                        move(thumb), move(thumb_hover), move(thumb_pressed), move(thumb_disabled)));
                    child_metadata->state = AssetMetadata::State::Loaded;
                    children.append(child_metadata->get_ref());
                } break;

                case StyleType::Slider: {
                    auto track = style->get_drawable_style("track"_h, {});
                    auto track_thickness = style->get_s32("trackThickness"_h, 3);
                    auto thumb = style->get_drawable_style("thumb"_h, {});
                    auto thumb_hover = style->get_drawable_style("thumbHover"_h, thumb);
                    auto thumb_pressed = style->get_drawable_style("thumbPressed"_h, thumb);
                    auto thumb_disabled = style->get_drawable_style("thumbDisabled"_h, thumb);
                    auto thumb_size = style->get_v2i("thumbSize"_h, v2i(8, 8));

                    AssetMetadata* child_metadata = asset_manager().add_asset(SliderStyle::asset_type(), style->name, {});
                    child_metadata->loaded_asset = adopt_own(*new SliderStyle(
                        move(track), move(track_thickness),
                        move(thumb), move(thumb_hover), move(thumb_pressed), move(thumb_disabled), move(thumb_size)));
                    child_metadata->state = AssetMetadata::State::Loaded;
                    children.append(child_metadata->get_ref());
                } break;

                case StyleType::TextInput: {
                    auto font = style->get_asset_ref<BitmapFont>("font"_h, default_font_name);
                    auto text_color = style->get_colour("textColor"_h, white);
                    auto text_alignment = style->get_alignment("textAlignment"_h, { HAlign::Left, VAlign::Top });
                    auto background = style->get_drawable_style("background"_h, {});
                    auto padding = style->get_padding("padding"_h, {});
                    auto show_caret = style->get_bool("showCaret"_h, true);
                    auto caret_flash_cycle_duration = style->get_float("caretFlashCycleDuration"_h, 1.0f);

                    AssetMetadata* child_metadata = asset_manager().add_asset(TextInputStyle::asset_type(), style->name, {});
                    child_metadata->loaded_asset = adopt_own(*new TextInputStyle(
                        move(font), move(text_color), move(text_alignment),
                        move(background), move(padding),
                        move(show_caret), move(caret_flash_cycle_duration)));
                    child_metadata->state = AssetMetadata::State::Loaded;
                    children.append(child_metadata->get_ref());
                } break;

                case StyleType::Window: {
                    auto title_label_style = style->get_asset_ref<LabelStyle>("titleLabelStyle"_h);
                    auto title_bar_height = style->get_s32("titleBarHeight"_h, 16);
                    auto title_bar_color = style->get_colour("titleBarColor"_h, Colour::from_rgb_255(128, 128, 128, 255));
                    auto title_bar_color_inactive = style->get_colour("titleBarColorInactive"_h, title_bar_color);
                    auto title_bar_button_hover_color = style->get_colour("titleBarButtonHoverColor"_h, transparent);
                    auto offset_from_mouse = style->get_v2i("offsetFromMouse"_h, {});
                    auto panel_style = style->get_asset_ref<PanelStyle>("panelStyle"_h);

                    AssetMetadata* child_metadata = asset_manager().add_asset(WindowStyle::asset_type(), style->name, {});
                    child_metadata->loaded_asset = adopt_own(*new WindowStyle(
                        move(title_label_style), move(title_bar_height), move(title_bar_color), move(title_bar_color_inactive), move(title_bar_button_hover_color),
                        move(offset_from_mouse), move(panel_style)));
                    child_metadata->state = AssetMetadata::State::Loaded;
                    children.append(child_metadata->get_ref());
                } break;

                    INVALID_DEFAULT_CASE;
                }
            }
        }
    }

    return { adopt_own(*new ContainerAsset(move(children))) };
}

}
