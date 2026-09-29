// SPDX-License-Identifier: Apache-2.0
// Epilogue screen: the run summary and a way back to the title.

#include "../ashes_common.hpp"
#include "../ashes_widgets.hpp"

#include "sdk/math.hpp"

namespace ashes {
namespace {

ids::Id EndingNameId(const GameContext& context) {
    switch (EndingDialogueFor(context.progress.flags)) {
        case kDlgEndGood:
            return ids::Id::kStoryEndGoodL2;
        case kDlgEndMid:
            return ids::Id::kStoryEndMidL2;
        default:
            return ids::Id::kStoryEndBadL2;
    }
}

micropixel::Rect PartyRowRect(const GameContext& context, uint8_t index, uint8_t count) {
    const micropixel::Rect body = context.layout.body;
    const int32_t pad = 8;
    const int32_t rows = count == 0U ? 1U : count;
    const int32_t height = micropixel::math::Max((body.height - pad * 2) / (rows + 2), 30);
    return {body.x + pad, body.y + pad + static_cast<int32_t>(index) * height, body.width - pad * 2, height - 4};
}

}  // namespace

void EndingSceneEnter(GameContext& context) {
    context.cursor = 0U;
    context.dirty = true;
    // The run is over: clearing the slot makes the title offer a fresh start.
    (void)context.app.storage().Remove(kSaveKey);
    context.has_save = false;
}

void EndingSceneUpdate(GameContext& context, uint32_t delta_ms) {
    (void)delta_ms;
    context.dirty = true;
}

bool EndingSceneTouch(GameContext& context, const micropixel::TouchEvent& touch) {
    if (touch.phase() != micropixel::TouchPhase::kDown) {
        return false;
    }
    PushScene(context, kSceneTitle);
    return true;
}

bool EndingSceneKey(GameContext& context, micropixel::KeyCode code) {
    switch (code) {
        case micropixel::KeyCode::kConfirm:
        case micropixel::KeyCode::kSouth:
        case micropixel::KeyCode::kBack:
        case micropixel::KeyCode::kMenu:
            PushScene(context, kSceneTitle);
            return true;
        default:
            return false;
    }
}

void EndingSceneRender(GameContext& context) {
    GameView& view = context.view;
    const micropixel::Rect screen = context.layout.screen;
    const micropixel::Rect body = context.layout.body;
    view.Begin();

    view.Fill(screen, theme::kInk);
    const int32_t glow = micropixel::math::Clamp<int32_t>(screen.width / 2, 120, 320);
    view.Round({screen.center_x() - glow / 2, screen.y + screen.height / 12, glow, glow},
               micropixel::Color::Rgb(20U, 26U, 48U), micropixel::Color::Rgb(44U, 58U, 96U),
               static_cast<uint32_t>(glow / 2), 1U);

    view.CenterText(screen.center_x(), screen.y + screen.height / 14, context.strings.Get(ids::Id::kUiEnding),
                    theme::kAccent, micropixel::SystemFont::kLarge);
    view.Fill({screen.center_x() - screen.width / 5, screen.y + screen.height * 13 / 100, screen.width * 2 / 5, 1},
              theme::kEdge);
    view.CenterText(screen.center_x(), screen.y + screen.height * 15 / 100, context.strings.Get(EndingNameId(context)),
                    theme::kMuted, micropixel::SystemFont::kSmall);

    // Party summary.
    const uint8_t count = micropixel::math::Min<uint8_t>(context.progress.party_size, kMaxParty);
    for (uint8_t index = 0U; index < count; ++index) {
        const PartyMember& member = context.progress.party[index];
        const micropixel::Rect row = PartyRowRect(context, index, count);
        widgets::Panel(view, row, theme::kPanel, theme::kEdge, 8U);
        widgets::Portrait(view, {row.x + 4, row.y + 4, row.height - 8, row.height - 8},
                          widgets::PortraitForCharacter(member.character), 3U, false);
        view.Text({row.x + row.height + 4, row.y + 5}, context.strings.Get(MemberNameId(member)), theme::kText,
                  micropixel::SystemFont::kSmall);
        Line stats;
        (void)stats.Append(context.strings.Get(ids::Id::kUiLv));
        (void)stats.Append(" ");
        (void)stats.AppendUint(member.level);
        (void)stats.Append("   ");
        (void)stats.Append(context.strings.Get(ids::Id::kUiHp));
        (void)stats.Append(" ");
        (void)stats.AppendUint(member.hp);
        (void)stats.Append("/");
        (void)stats.AppendUint(MemberMaxHp(member));
        view.Text({row.x + row.height + 4, row.y + row.height / 2}, stats.c_str(), theme::kMuted,
                  micropixel::SystemFont::kSmall);
    }

    // Run totals.
    const micropixel::Rect totals = PartyRowRect(context, count, static_cast<uint8_t>(count + 1U));
    widgets::Panel(view, totals, theme::kPanelDeep, theme::kEdge, 8U);
    Line summary;
    (void)summary.Append(context.strings.Get(ids::Id::kUiGold));
    (void)summary.Append(" ");
    (void)summary.AppendUint(context.progress.gold);
    view.CenterText(totals.center_x(), totals.y + 6, summary.c_str(), theme::kAccent,
                    micropixel::SystemFont::kMedium);

    (void)body;
    view.CenterText(screen.center_x(), screen.y + screen.height - 20, context.strings.Get(ids::Id::kUiEndingHint),
                    theme::kDim, micropixel::SystemFont::kSmall);
    view.End();
}

}  // namespace ashes
