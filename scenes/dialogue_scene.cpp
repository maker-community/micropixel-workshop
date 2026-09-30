// SPDX-License-Identifier: Apache-2.0
// Dialogue scene: portrait, typewriter text and the branching choice list.

#include "../pal_common.hpp"
#include "../pal_widgets.hpp"

#include "sdk/math.hpp"

namespace pal {
namespace {

inline constexpr uint32_t kMaxWrapLines = 4U;
inline constexpr int32_t kChoiceHeight = 24;
inline constexpr int32_t kChoiceGap = 3;

// Labels do not wrap, so a line longer than the box would run off the panel.
// Text is wrapped here into byte ranges of the original string.
struct Wrapped final {
    uint16_t start[kMaxWrapLines]{};
    uint16_t bytes[kMaxWrapLines]{};
    uint8_t count{};
};

struct DialogueView final {
    micropixel::Rect box{};
    Wrapped wrapped{};
    int32_t line_height{};
    micropixel::Rect choices[kMaxChoices]{};
};

struct GlyphSize final {
    int32_t wide{};
    int32_t height{};
};

// A CJK glyph's advance and the line height, measured once per font.
GlyphSize MeasureGlyph(GameContext& context) {
    static GlyphSize cached{};
    if (cached.wide == 0) {
        const auto measured = context.app.renderer().MeasureText("汉", micropixel::SystemFont::kMedium);
        cached.wide = measured.has_value() ? micropixel::math::Max<int32_t>(static_cast<int32_t>(measured.value().width), 8) : 18;
        cached.height = measured.has_value() ? micropixel::math::Max<int32_t>(static_cast<int32_t>(measured.value().height), 12) : 22;
    }
    return cached;
}

Wrapped WrapText(const char* text, int32_t max_width, int32_t wide) {
    Wrapped out{};
    if (text == nullptr) {
        return out;
    }
    uint32_t index = 0U;
    uint32_t line_start = 0U;
    int32_t width = 0;
    while (text[index] != '\0') {
        const uint8_t lead = static_cast<uint8_t>(text[index]);
        uint32_t length = lead < 0x80U ? 1U : (lead < 0xE0U ? 2U : (lead < 0xF0U ? 3U : 4U));
        for (uint32_t probe = 1U; probe < length; ++probe) {
            if (text[index + probe] == '\0') {
                length = probe;  // truncated sequence: stop at the terminator
                break;
            }
        }
        const int32_t advance = lead < 0x80U ? wide / 2 : wide;
        if (width + advance > max_width && index > line_start && out.count + 1U < kMaxWrapLines) {
            out.start[out.count] = static_cast<uint16_t>(line_start);
            out.bytes[out.count] = static_cast<uint16_t>(index - line_start);
            ++out.count;
            line_start = index;
            width = 0;
        }
        width += advance;
        index += length;
    }
    out.start[out.count] = static_cast<uint16_t>(line_start);
    out.bytes[out.count] = static_cast<uint16_t>(index - line_start);
    ++out.count;
    return out;
}

DialogueView BuildView(GameContext& context) {
    const GameLayout& layout = context.layout;
    const DialogueNodeDef& node = DialogueCurrent(context.dialogue);
    DialogueView result{};
    const GlyphSize glyph = MeasureGlyph(context);
    result.line_height = glyph.height + 2;
    if (context.dialogue.line < node.line_count) {
        result.wrapped = WrapText(context.strings.Get(node.lines[context.dialogue.line].text),
                                  layout.dialogue_text.width, glyph.wide);
    }
    const int32_t text_h = 8 + static_cast<int32_t>(result.wrapped.count) * result.line_height;
    int32_t needed = text_h + 22;  // room for the advance hint
    if (context.dialogue.choosing) {
        needed = text_h + 6 + static_cast<int32_t>(node.choice_count) * (kChoiceHeight + kChoiceGap) + 8;
    }
    const int32_t height = micropixel::math::Max(layout.dialogue.height, needed);
    result.box = {layout.dialogue.x, layout.dialogue.y + layout.dialogue.height - height, layout.dialogue.width,
                  height};
    const int32_t first_row = result.box.y + text_h + 6;
    for (uint8_t index = 0U; index < node.choice_count && index < kMaxChoices; ++index) {
        result.choices[index] = {result.box.x + 8, first_row + static_cast<int32_t>(index) * (kChoiceHeight + kChoiceGap),
                                 result.box.width - 16, kChoiceHeight};
    }
    return result;
}

void FinishOrContinue(GameContext& context) {
    if (context.dialogue.active) {
        return;  // a branch kept the conversation running
    }
    switch (context.dialogue_return) {
        case kDialogueToExplore:
            PushScene(context, kSceneExplore);
            break;
        case kDialogueToEnding:
            PushScene(context, kSceneEnding);
            break;
        default:
            AdvanceScript(context);
            break;
    }
}

void Confirm(GameContext& context) {
    context.audio.PlaySfx(SfxId::kCursor);
    if (DialogueConfirm(context.dialogue, context.strings)) {
        FinishOrContinue(context);
    }
    context.dirty = true;
}

micropixel::Rect ChoiceRect(GameContext& context, uint8_t index) {
    return BuildView(context).choices[index < kMaxChoices ? index : 0U];
}

}  // namespace

void DialogueSceneEnter(GameContext& context) {
    context.dialogue.selected = 0U;
    context.dirty = true;
}

void DialogueSceneUpdate(GameContext& context, uint32_t delta_ms) {
    // Only the typewriter changes the frame between taps.
    const uint32_t before = DialogueRevealed(context.dialogue);
    DialogueUpdate(context.dialogue, delta_ms);
    if (DialogueRevealed(context.dialogue) != before) {
        context.dirty = true;
    }
}

bool DialogueSceneTouch(GameContext& context, const micropixel::TouchEvent& touch) {
    if (touch.phase() != micropixel::TouchPhase::kDown) {
        return false;
    }
    if (!context.dialogue.choosing) {
        Confirm(context);
        return true;
    }
    const DialogueNodeDef& node = DialogueCurrent(context.dialogue);
    for (uint8_t index = 0U; index < node.choice_count; ++index) {
        if (ChoiceRect(context, index).contains(touch.position())) {
            context.dialogue.selected = index;
            context.audio.PlaySfx(SfxId::kConfirm);
            DialoguePick(context.dialogue, context.strings, context.progress.flags);
            FinishOrContinue(context);
            context.dirty = true;
            return true;
        }
    }
    return true;
}

bool DialogueSceneKey(GameContext& context, micropixel::KeyCode code) {
    if (context.dialogue.choosing) {
        switch (code) {
            case micropixel::KeyCode::kUp:
            case micropixel::KeyCode::kLeft:
                DialogueMove(context.dialogue, -1);
                context.audio.PlaySfx(SfxId::kCursor);
                context.dirty = true;
                return true;
            case micropixel::KeyCode::kDown:
            case micropixel::KeyCode::kRight:
                DialogueMove(context.dialogue, 1);
                context.audio.PlaySfx(SfxId::kCursor);
                context.dirty = true;
                return true;
            case micropixel::KeyCode::kConfirm:
            case micropixel::KeyCode::kSouth:
                context.audio.PlaySfx(SfxId::kConfirm);
                DialoguePick(context.dialogue, context.strings, context.progress.flags);
                FinishOrContinue(context);
                context.dirty = true;
                return true;
            default:
                return false;
        }
    }
    switch (code) {
        case micropixel::KeyCode::kConfirm:
        case micropixel::KeyCode::kSouth:
        case micropixel::KeyCode::kBack:
        case micropixel::KeyCode::kMenu:
            Confirm(context);
            return true;
        default:
            return false;
    }
}

void DialogueSceneRender(GameContext& context) {
    GameView& view = context.view;
    const GameLayout& layout = context.layout;
    const DialogueNodeDef& node = DialogueCurrent(context.dialogue);
    view.Begin();

    // Backdrop: a quiet night sky — a few faint stars and one muted moon, kept
    // dim so the text always leads.
    view.Fill(layout.screen, theme::kInk);
    const uint32_t stars[][2] = {{12U, 9U}, {26U, 16U}, {41U, 7U}, {58U, 13U}, {72U, 20U}, {88U, 8U}};
    for (const auto& star : stars) {
        const int32_t sx = layout.screen.x + layout.screen.width * static_cast<int32_t>(star[0]) / 100;
        const int32_t sy = layout.screen.y + layout.screen.height * static_cast<int32_t>(star[1]) / 100;
        view.Fill({sx, sy, 2, 2}, theme::kMuted, 160U);
    }
    const int32_t moon = micropixel::math::Clamp<int32_t>(layout.screen.width / 8, 28, 58);
    view.Round({layout.screen.x + layout.screen.width - moon - 14, layout.screen.y + 12, moon, moon},
               micropixel::Color::Rgb(48U, 54U, 80U), micropixel::Color::Rgb(74U, 84U, 120U),
               static_cast<uint32_t>(moon / 2), 1U);

    if (context.dialogue.line < node.line_count) {
        const DialogueLineDef& line = node.lines[context.dialogue.line];
        const DialogueView frame = BuildView(context);
        const bool choosing = context.dialogue.choosing;

        // Speaker portrait, dimmed for the narrator. The choice list hides it:
        // the enlarged box would sit on top of it anyway.
        if (line.portrait != 0xFFU && !choosing) {
            widgets::Portrait(view, layout.portrait, line.portrait, line.emotion, true);
            const micropixel::Rect tag{layout.portrait.x + layout.portrait.width + 8, layout.dialogue.y - 22, 132, 18};
            widgets::NameTag(view, tag, context.strings.Get(line.speaker), theme::kAccent);
        }

        // Text box.
        widgets::Panel(view, frame.box, theme::kPanel, theme::kEdge, 12U);

        const char* full = context.strings.Get(line.text);
        uint32_t remaining = DialogueRevealed(context.dialogue);
        for (uint8_t row = 0U; row < frame.wrapped.count && remaining > 0U; ++row) {
            char piece[220]{};
            const uint32_t bytes = micropixel::math::Min<uint32_t>(frame.wrapped.bytes[row], sizeof(piece) - 1U);
            for (uint32_t byte = 0U; byte < bytes; ++byte) {
                piece[byte] = full[frame.wrapped.start[row] + byte];
            }
            const uint32_t codepoints = Utf8Count(piece);
            char shown[220]{};
            Utf8CopyPrefix(shown, sizeof(shown), piece, remaining);
            view.Text({layout.dialogue_text.x, frame.box.y + 8 + static_cast<int32_t>(row) * frame.line_height},
                      shown, theme::kText, micropixel::SystemFont::kMedium);
            remaining -= micropixel::math::Min(remaining, codepoints);
        }

        if (choosing) {
            for (uint8_t index = 0U; index < node.choice_count; ++index) {
                const micropixel::Rect row = frame.choices[index];
                const bool focused = index == context.dialogue.selected;
                view.Round(row, focused ? theme::kPanelDeep : theme::kPanel,
                           focused ? theme::kAccent : theme::kEdge, 8U, focused ? 2U : 1U);
                view.CenterText(row.center_x(), row.center_y() - 7, context.strings.Get(node.choices[index].text),
                                focused ? theme::kHighlight : theme::kMuted, micropixel::SystemFont::kSmall);
            }
        } else if (DialogueLineDone(context.dialogue)) {
            view.CenterText(frame.box.center_x(), frame.box.y + frame.box.height - 18,
                            context.strings.Get(ids::Id::kUiAdvance), theme::kDim, micropixel::SystemFont::kSmall);
        }
    }

    view.End();
}

}  // namespace pal
