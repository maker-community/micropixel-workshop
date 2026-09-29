// SPDX-License-Identifier: Apache-2.0
// Chapter exploration: a grid map, a diamond pad and NPC interaction.

#include "../pal_common.hpp"
#include "../pal_widgets.hpp"

#include "sdk/math.hpp"

namespace pal {
namespace {

inline constexpr uint8_t kDirUp = 0U;
inline constexpr uint8_t kDirLeft = 1U;
inline constexpr uint8_t kDirRight = 2U;

// Triangle built from stacked bars (the SDK has no rotated primitives). The
// apex points along `direction`: the bars widen away from it.
void Arrow(GameView& view, micropixel::Rect area, uint8_t direction) {
    const int32_t size = micropixel::math::Min(area.width, area.height) * 3 / 5;
    if (size < 6) {
        return;
    }
    constexpr int32_t kSteps = 5;
    const int32_t step = micropixel::math::Max(size / kSteps, 1);
    const int32_t half = size / 2;
    const int32_t cx = area.center_x();
    const int32_t cy = area.center_y();
    for (int32_t index = 0; index < kSteps; ++index) {
        const int32_t span = (index + 1) * size / kSteps;
        const int32_t offset = index * step;
        micropixel::Rect bar{};
        switch (direction) {
            case kDirUp:
                bar = {cx - span / 2, cy - half + offset, span, step};
                break;
            case kDirLeft:
                bar = {cx - half + offset, cy - span / 2, step, span};
                break;
            case kDirRight:
                bar = {cx + half - offset - step, cy - span / 2, step, span};
                break;
            default:
                bar = {cx - span / 2, cy + half - offset - step, span, step};
                break;
        }
        view.Fill(bar, theme::kText);
    }
}

void NpcToken(GameView& view, micropixel::Rect area, uint8_t portrait) {
    micropixel::Color robe = micropixel::Color::Rgb(104U, 96U, 84U);
    micropixel::Color hair = micropixel::Color::Rgb(206U, 206U, 212U);
    switch (portrait) {
        case 2U:
            robe = micropixel::Color::Rgb(120U, 124U, 148U);
            break;
        case 3U:
            robe = micropixel::Color::Rgb(92U, 72U, 60U);
            break;
        default:
            robe = micropixel::Color::Rgb(126U, 104U, 74U);
            hair = micropixel::Color::Rgb(52U, 42U, 36U);
            break;
    }
    const int32_t w = area.width;
    const int32_t h = area.height;
    const int32_t cx = area.x + w / 2;
    view.Round({area.x + w / 6, area.y + h - h / 8, w * 2 / 3, h / 10}, micropixel::Color::Black(),
               micropixel::Color::Black(), h / 12, 0U, 90U);
    view.Round({area.x + w / 5, area.y + h / 2, w * 3 / 5, h * 2 / 5}, robe, robe.Darkened(50U), w / 8, 1U);
    const int32_t head = micropixel::math::Max(w * 3 / 5, 6);
    view.Round({cx - head / 2, area.y + h / 6, head, head}, micropixel::Color::Rgb(246U, 224U, 206U),
               micropixel::Color::Rgb(206U, 180U, 160U), head / 2, 1U);
    view.Round({cx - head / 2 - w / 20, area.y + h / 8, head + w / 10, head / 2}, hair, hair.Lightened(50U),
               head / 3, 1U);
}

void DrawMap(GameContext& context) {
    GameView& view = context.view;
    const micropixel::Rect stage = context.layout.stage;
    const uint8_t map_id = context.world.map;
    const MapDef& map = Map(map_id);
    const int32_t tile = micropixel::math::Max(micropixel::math::Min(stage.width / 9, stage.height / 7), 12);
    const int32_t view_cols = micropixel::math::Max(stage.width / tile, 1);
    const int32_t view_rows = micropixel::math::Max(stage.height / tile, 1);

    int32_t cam_x16 = WorldRenderX16(context.world) + 8 - view_cols * 8;
    int32_t cam_y16 = WorldRenderY16(context.world) + 8 - view_rows * 8;
    const int32_t span_x16 = static_cast<int32_t>(map.width) * 16 - view_cols * 16;
    const int32_t span_y16 = static_cast<int32_t>(map.height) * 16 - view_rows * 16;
    cam_x16 = span_x16 <= 0 ? span_x16 / 2 : micropixel::math::Clamp<int32_t>(cam_x16, 0, span_x16);
    cam_y16 = span_y16 <= 0 ? span_y16 / 2 : micropixel::math::Clamp<int32_t>(cam_y16, 0, span_y16);

    view.Fill(stage, theme::kInk);

    for (int32_t row = 0; row < static_cast<int32_t>(map.height); ++row) {
        for (int32_t col = 0; col < static_cast<int32_t>(map.width); ++col) {
            const int32_t sx = stage.x + (col * 16 - cam_x16) * tile / 16;
            const int32_t sy = stage.y + (row * 16 - cam_y16) * tile / 16;
            if (sx >= stage.x + stage.width || sy >= stage.y + stage.height || sx + tile <= stage.x ||
                sy + tile <= stage.y) {
                continue;
            }
            widgets::DrawTile(view, {sx, sy, tile + 1, tile + 1},
                              static_cast<uint8_t>(MapTileAt(map_id, col, row)));
        }
    }

    // Chapter exits pulse so the player always knows where to walk.
    const uint32_t pulse = context.progress.play_seconds % 2U;
    for (uint8_t index = 0U; index < map.trigger_count; ++index) {
        const MapTriggerDef& trigger = map.triggers[index];
        for (uint8_t row = 0U; row < trigger.height; ++row) {
            for (uint8_t col = 0U; col < trigger.width; ++col) {
                const int32_t sx = stage.x + ((trigger.x + col) * 16 - cam_x16) * tile / 16;
                const int32_t sy = stage.y + ((trigger.y + row) * 16 - cam_y16) * tile / 16;
                view.Round({sx + 2, sy + 2, tile - 4, tile - 4},
                           pulse == 0U ? theme::kShrine.Lightened(40U) : theme::kShrine,
                           theme::kHighlight, static_cast<uint32_t>(tile / 6), 2U);
            }
        }
    }

    // NPCs with a floating name tag.
    for (uint8_t index = 0U; index < map.npc_count; ++index) {
        const MapNpcDef& npc = map.npcs[index];
        const int32_t sx = stage.x + (static_cast<int32_t>(npc.x) * 16 - cam_x16) * tile / 16;
        const int32_t sy = stage.y + (static_cast<int32_t>(npc.y) * 16 - cam_y16) * tile / 16;
        NpcToken(view, {sx + 2, sy + 1, tile - 4, tile - 2}, npc.portrait);
        const int32_t tag_w = micropixel::math::Min(tile * 2 + 8, 76);
        widgets::NameTag(view, {sx + tile / 2 - tag_w / 2, sy - 14, tag_w, 13},
                         context.strings.Get(npc.name), theme::kAccent);
    }

    // The party leader.
    const int32_t px = stage.x + (WorldRenderX16(context.world) - cam_x16) * tile / 16;
    const int32_t py = stage.y + (WorldRenderY16(context.world) - cam_y16) * tile / 16;
    const uint8_t leader = context.progress.party_size > 0U ? context.progress.party[0].character : kCharXiao;
    widgets::HeroToken(view, {px + 2, py + 1, tile - 4, tile - 2}, leader, context.world.facing);
}

void TryStep(GameContext& context, int32_t dx, int32_t dy) {
    if (WorldStep(context.world, dx, dy) != WorldStepResult::kNone) {
        context.dirty = true;
    }
}

}  // namespace

void ExploreSceneEnter(GameContext& context) {
    context.cursor = 0U;
    context.dirty = true;
}

void ExploreSceneUpdate(GameContext& context, uint32_t delta_ms) {
    WorldUpdate(context.world, delta_ms);

    // Keep the save record pointing at where the party actually stands.
    if (context.progress.player_x != context.world.x || context.progress.player_y != context.world.y) {
        LogTrace(context, "tile", context.world.x, context.world.y);
    }
    context.progress.map_id = context.world.map;
    context.progress.player_x = context.world.x;
    context.progress.player_y = context.world.y;

    if (context.world.pending_dialogue != kEmptySlot) {
        const uint8_t node = context.world.pending_dialogue;
        context.world.pending_dialogue = kEmptySlot;
        PushDialogue(context, node, kDialogueToExplore);
        return;
    }
    if (context.world.completed) {
        context.world.completed = false;
        AdvanceScript(context);
        return;
    }
    context.dirty = true;
}

bool ExploreSceneTouch(GameContext& context, const micropixel::TouchEvent& touch) {
    if (touch.phase() != micropixel::TouchPhase::kDown) {
        return false;
    }
    const GameLayout& layout = context.layout;
    for (uint8_t index = 0U; index < 4U; ++index) {
        if (!layout.dpad[index].contains(touch.position())) {
            continue;
        }
        switch (index) {
            case kDirUp:
                TryStep(context, 0, -1);
                break;
            case kDirLeft:
                TryStep(context, -1, 0);
                break;
            case kDirRight:
                TryStep(context, 1, 0);
                break;
            default:
                TryStep(context, 0, 1);
                break;
        }
        return true;
    }
    if (layout.menu_button.contains(touch.position())) {
        context.previous_scene = kSceneExplore;
        PushScene(context, kSceneMenu);
        return true;
    }
    return false;
}

bool ExploreSceneKey(GameContext& context, micropixel::KeyCode code) {
    switch (code) {
        case micropixel::KeyCode::kUp:
            TryStep(context, 0, -1);
            return true;
        case micropixel::KeyCode::kDown:
            TryStep(context, 0, 1);
            return true;
        case micropixel::KeyCode::kLeft:
            TryStep(context, -1, 0);
            return true;
        case micropixel::KeyCode::kRight:
            TryStep(context, 1, 0);
            return true;
        case micropixel::KeyCode::kMenu:
        case micropixel::KeyCode::kBack:
            context.previous_scene = kSceneExplore;
            PushScene(context, kSceneMenu);
            return true;
        default:
            return false;
    }
}

void ExploreSceneRender(GameContext& context) {
    GameView& view = context.view;
    const GameLayout& layout = context.layout;
    // The map layer is clipped to the viewport, so tiles, exit markers and
    // tokens can never bleed into the header, the pad or the button below.
    view.Begin(layout.stage);
    view.UseMapLayer(true);
    DrawMap(context);
    view.UseMapLayer(false);

    widgets::Header(context, ContextMapNameId(context), true);

    for (uint8_t index = 0U; index < 4U; ++index) {
        view.Round(layout.dpad[index], theme::kPanel, theme::kEdge, 8U, 1U);
        Arrow(view, layout.dpad[index], index);
    }
    const micropixel::Rect menu = layout.menu_button;
    view.Round(menu, theme::kPanel, theme::kEdge, 8U, 1U);
    view.CenterText(menu.center_x(), menu.center_y() - 7, context.strings.Get(ids::Id::kUiStatus), theme::kText,
                    micropixel::SystemFont::kSmall);

    // Hint pill floating over the bottom of the viewport so it stays legible
    // whatever tile is under it.
    const int32_t hint_w = micropixel::math::Min(layout.stage.width - 12, 214);
    const micropixel::Rect hint{layout.stage.center_x() - hint_w / 2,
                               layout.stage.y + layout.stage.height - 20, hint_w, 17};
    view.Round(hint, theme::kPanelDeep, theme::kEdge, 8U, 1U, 220U);
    view.CenterText(hint.center_x(), hint.y + 2, context.strings.Get(ids::Id::kUiExploreHint), theme::kMuted,
                    micropixel::SystemFont::kSmall);

    view.End();
}

}  // namespace pal
