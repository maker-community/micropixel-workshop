// SPDX-License-Identifier: MIT
// Title screen: night sky, moon, mountain ridge and the two entry points.

#include "../pal_common.hpp"
#include "../pal_widgets.hpp"

#include "sdk/math.hpp"

namespace pal {
namespace {

inline constexpr uint8_t kTitleNew = 0U;
inline constexpr uint8_t kTitleContinue = 1U;
inline constexpr uint8_t kTitleItems = 2U;

micropixel::Rect TitleItemRect(const GameContext& context, uint8_t index) {
    const micropixel::Rect screen = context.layout.screen;
    const int32_t width = micropixel::math::Min(screen.width - 56, 280);
    const int32_t height = micropixel::math::Clamp<int32_t>(screen.height / 11, 28, 46);
    const int32_t gap = 12;
    const int32_t total = static_cast<int32_t>(kTitleItems) * height + gap;
    const int32_t top = screen.y + screen.height * 63 / 100 - total / 2;
    return {screen.center_x() - width / 2, top + static_cast<int32_t>(index) * (height + gap), width, height};
}

void Backdrop(GameContext& context) {
    GameView& view = context.view;
    const micropixel::Rect screen = context.layout.screen;
    view.Fill(screen, theme::kInk);

    const uint32_t stars[][2] = {{7U, 5U},  {21U, 11U}, {36U, 4U},  {53U, 9U},  {69U, 6U},  {85U, 13U},
                                 {13U, 21U}, {29U, 27U}, {47U, 19U}, {63U, 25U}, {79U, 23U}, {91U, 29U}};
    for (uint32_t index = 0U; index < 12U; ++index) {
        const int32_t x = screen.x + screen.width * static_cast<int32_t>(stars[index][0]) / 100;
        const int32_t y = screen.y + screen.height * static_cast<int32_t>(stars[index][1]) / 100;
        const bool bright = index % 3U == 0U;
        view.Fill({x, y, bright ? 3 : 2, bright ? 3 : 2}, bright ? theme::kAccent : theme::kMuted);
    }

    // Moon tucked into the top-right corner so it never crowds the title block.
    const int32_t moon = micropixel::math::Clamp<int32_t>(screen.width / 9, 26, 54);
    view.Round({screen.x + screen.width - moon - 6, screen.y + 4, moon, moon},
               micropixel::Color::Rgb(238U, 232U, 208U), micropixel::Color::Rgb(198U, 192U, 168U),
               static_cast<uint32_t>(moon / 2), 1U);

    // Pine treeline silhouetted along the bottom edge, below the menu.
    const int32_t ground = screen.y + screen.height * 90 / 100;
    view.Fill({screen.x, ground, screen.width, screen.y + screen.height - ground},
              micropixel::Color::Rgb(10U, 14U, 28U));
    const uint32_t trees[][3] = {{4U, 14U, 9U}, {11U, 20U, 7U}, {19U, 12U, 8U}, {27U, 17U, 6U},
                                 {35U, 22U, 9U}, {45U, 15U, 7U}, {54U, 19U, 8U}, {63U, 13U, 6U},
                                 {71U, 21U, 9U}, {80U, 16U, 7U}, {88U, 12U, 8U}, {95U, 18U, 6U}};
    for (const auto& tree : trees) {
        const int32_t height = screen.height * static_cast<int32_t>(tree[1]) / 100;
        const int32_t width =
            micropixel::math::Max(screen.width * static_cast<int32_t>(tree[2]) / 100, 6);
        const int32_t cx = screen.x + screen.width * static_cast<int32_t>(tree[0]) / 100;
        view.Round({cx - width / 2, ground - height, width, height + 14}, micropixel::Color::Rgb(15U, 25U, 34U),
                   micropixel::Color::Rgb(24U, 40U, 50U), static_cast<uint32_t>(width / 2), 1U);
    }
}

void StartNewGame(GameContext& context) {
    ProgressReset(context.progress);
    context.progress.script_index = 0U;
    StoreProgress(context);
    EnterScriptStep(context);
}

void ContinueGame(GameContext& context) {
    if (!ProgressLoad(context.progress, context.app.storage())) {
        StartNewGame(context);
        return;
    }
    PartySyncFromFlags(context.progress);
    EnterScriptStep(context);
}

void Activate(GameContext& context, uint8_t index) {
    const bool start_new = index == kTitleNew;
    if (!start_new && !context.has_save) {
        return;  // 继续前缘 stays dead until there is something to continue
    }
    context.audio.PlaySfx(SfxId::kConfirm);
    if (start_new) {
        StartNewGame(context);
        return;
    }
    ContinueGame(context);
}

}  // namespace

void TitleSceneEnter(GameContext& context) {
    context.has_save = ProgressHasSave(context.app.storage());
    context.cursor = context.has_save ? kTitleContinue : kTitleNew;
    context.dirty = true;
}

void TitleSceneUpdate(GameContext& context, uint32_t delta_ms) {
    (void)context;  // the backdrop is static; redraws come from input
    (void)delta_ms;
}

bool TitleSceneTouch(GameContext& context, const micropixel::TouchEvent& touch) {
    if (touch.phase() != micropixel::TouchPhase::kDown) {
        return false;
    }
    for (uint8_t index = 0U; index < kTitleItems; ++index) {
        if (TitleItemRect(context, index).contains(touch.position())) {
            context.cursor = index;
            Activate(context, index);
            return true;
        }
    }
    return false;
}

bool TitleSceneKey(GameContext& context, micropixel::KeyCode code) {
    switch (code) {
        case micropixel::KeyCode::kUp:
        case micropixel::KeyCode::kLeft:
            context.cursor = context.cursor == 0U ? 1U : 0U;
            context.audio.PlaySfx(SfxId::kCursor);
            context.dirty = true;
            return true;
        case micropixel::KeyCode::kDown:
        case micropixel::KeyCode::kRight:
            context.cursor = context.cursor == 0U ? 1U : 0U;
            context.audio.PlaySfx(SfxId::kCursor);
            context.dirty = true;
            return true;
        case micropixel::KeyCode::kConfirm:
        case micropixel::KeyCode::kSouth:
            Activate(context, context.cursor);
            return true;
        default:
            return false;
    }
}

void TitleSceneRender(GameContext& context) {
    GameView& view = context.view;
    const micropixel::Rect screen = context.layout.screen;
    view.Begin();

    Backdrop(context);

    // Title block.
    const int32_t title_y = screen.y + screen.height * 24 / 100;
    view.CenterText(screen.center_x(), title_y, context.strings.Get(ids::Id::kAppTitle), theme::kHighlight,
                    micropixel::SystemFont::kTitle);
    view.CenterText(screen.center_x(), title_y + screen.height * 9 / 100,
                    context.strings.Get(ids::Id::kAppSubtitle), theme::kMuted, micropixel::SystemFont::kSmall);
    view.Fill({screen.center_x() - screen.width / 5, title_y + screen.height * 7 / 100, screen.width * 2 / 5, 1},
              theme::kEdge);

    for (uint8_t index = 0U; index < kTitleItems; ++index) {
        const micropixel::Rect item = TitleItemRect(context, index);
        const bool enabled = index == kTitleNew || context.has_save;
        const bool focused = context.cursor == index && enabled;
        view.Round(item, focused ? theme::kPanel : theme::kPanelDeep,
                   focused ? theme::kAccent : theme::kEdge, 10U, focused ? 2U : 1U);
        const char* label = index == kTitleNew ? context.strings.Get(ids::Id::kUiNewGame)
                                               : context.strings.Get(ids::Id::kUiContinue);
        view.CenterText(item.center_x(), item.center_y() - 7, label,
                        enabled ? (focused ? theme::kHighlight : theme::kText) : theme::kDim,
                        micropixel::SystemFont::kMedium);
    }

    view.End();
}

}  // namespace pal
