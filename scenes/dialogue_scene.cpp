// SPDX-License-Identifier: Apache-2.0
// Dialogue scene: portrait, typewriter text and the branching choice list.

#include "../ashes_common.hpp"
#include "../ashes_widgets.hpp"

#include "sdk/math.hpp"

namespace ashes {
namespace {

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
    if (DialogueConfirm(context.dialogue, context.strings)) {
        FinishOrContinue(context);
    }
    context.dirty = true;
}

micropixel::Rect ChoiceRect(const GameContext& context, uint8_t index) {
    const DialogueNodeDef& node = DialogueCurrent(context.dialogue);
    const micropixel::Rect area = context.layout.choices;
    const uint8_t count = micropixel::math::Max<uint8_t>(node.choice_count, 1U);
    const int32_t height = micropixel::math::Max(area.height / count, 14);
    return {area.x, area.y + static_cast<int32_t>(index) * height, area.width, height - 2};
}

}  // namespace

void DialogueSceneEnter(GameContext& context) {
    context.dialogue.selected = 0U;
    context.dirty = true;
}

void DialogueSceneUpdate(GameContext& context, uint32_t delta_ms) {
    DialogueUpdate(context.dialogue, delta_ms);
    context.dirty = true;
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
                context.dirty = true;
                return true;
            case micropixel::KeyCode::kDown:
            case micropixel::KeyCode::kRight:
                DialogueMove(context.dialogue, 1);
                context.dirty = true;
                return true;
            case micropixel::KeyCode::kConfirm:
            case micropixel::KeyCode::kSouth:
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

        // Speaker portrait, dimmed for the narrator.
        if (line.portrait != 0xFFU) {
            widgets::Portrait(view, layout.portrait, line.portrait, line.emotion, true);
            const micropixel::Rect tag{layout.portrait.x + layout.portrait.width + 8, layout.dialogue.y - 22, 132, 18};
            widgets::NameTag(view, tag, context.strings.Get(line.speaker), theme::kAccent);
        }

        // Text box.
        widgets::Panel(view, layout.dialogue, theme::kPanel, theme::kEdge, 12U);

        char buffer[220]{};
        Utf8CopyPrefix(buffer, sizeof(buffer), context.strings.Get(line.text), DialogueRevealed(context.dialogue));
        view.Text({layout.dialogue_text.x, layout.dialogue_text.y}, buffer, theme::kText,
                  micropixel::SystemFont::kMedium);

        if (context.dialogue.choosing) {
            for (uint8_t index = 0U; index < node.choice_count; ++index) {
                const micropixel::Rect row = ChoiceRect(context, index);
                const bool focused = index == context.dialogue.selected;
                view.Round(row, focused ? theme::kPanelDeep : theme::kPanel,
                           focused ? theme::kAccent : theme::kEdge, 8U, focused ? 2U : 1U);
                view.CenterText(row.center_x(), row.center_y() - 6, context.strings.Get(node.choices[index].text),
                                focused ? theme::kHighlight : theme::kMuted, micropixel::SystemFont::kSmall);
            }
        } else if (DialogueLineDone(context.dialogue)) {
            const micropixel::Rect hint{layout.dialogue.x, layout.dialogue.y + layout.dialogue.height - 18,
                                        layout.dialogue.width - 10, 14};
            view.CenterText(hint.center_x(), hint.y, context.strings.Get(ids::Id::kUiAdvance), theme::kDim,
                            micropixel::SystemFont::kSmall);
        }
    }

    view.End();
}

}  // namespace ashes
