// SPDX-License-Identifier: Apache-2.0
// Battle scene: foes, party cards, the message strip and the command panel.

#include "../pal_common.hpp"
#include "../pal_widgets.hpp"

#include "sdk/math.hpp"

namespace pal {
namespace {

inline constexpr uint8_t kRootAttack = 0U;
inline constexpr uint8_t kRootSkill = 1U;
inline constexpr uint8_t kRootItem = 2U;
inline constexpr uint8_t kRootDefend = 3U;
inline constexpr uint8_t kRootFlee = 4U;

ids::Id RootLabel(uint8_t row) {
    switch (row) {
        case kRootAttack:
            return ids::Id::kUiAttack;
        case kRootSkill:
            return ids::Id::kUiSkill;
        case kRootItem:
            return ids::Id::kUiItem;
        case kRootDefend:
            return ids::Id::kUiDefend;
        case kRootFlee:
            return ids::Id::kUiFlee;
        default:
            return ids::Id::kUiAttack;
    }
}

ids::Id UnitNameId(const Combatant& unit) {
    return unit.enemy ? Enemy(unit.species).name : CharacterNameId(unit.species);
}

uint8_t CollectEnemies(const GameContext& context, uint8_t* out) {
    uint8_t count = 0U;
    for (uint8_t index = 0U; index < context.battle.unit_count; ++index) {
        if (BattleUnit(context.battle, index).enemy) {
            out[count++] = index;
        }
    }
    return count;
}

uint8_t CollectParty(const GameContext& context, uint8_t* out) {
    uint8_t count = 0U;
    for (uint8_t index = 0U; index < context.battle.unit_count; ++index) {
        if (!BattleUnit(context.battle, index).enemy) {
            out[count++] = index;
        }
    }
    return count;
}

micropixel::Rect EnemyRect(const GameContext& context, uint8_t slot, uint8_t slots) {
    const micropixel::Rect area = context.layout.enemy_area;
    const int32_t count = slots == 0U ? 1 : slots;
    const int32_t cell = area.width / count;
    const int32_t width = micropixel::math::Max(micropixel::math::Min(cell - 8, 120), 40);
    const int32_t height = micropixel::math::Max(area.height - 6, 40);
    return {area.x + static_cast<int32_t>(slot) * cell + (cell - width) / 2, area.y + 3, width, height};
}

// Square the foe's art is drawn in, between the name caption and the HP bar.
micropixel::Rect EnemyArt(const micropixel::Rect& plate) {
    const int32_t side = micropixel::math::Max(micropixel::math::Min(plate.width - 12, plate.height - 29), 16);
    return {plate.center_x() - side / 2, plate.y + 18, side, side};
}

micropixel::Rect PartyRect(const GameContext& context, uint8_t slot, uint8_t slots) {
    const micropixel::Rect area = context.layout.party_area;
    const int32_t pad = 3;
    const int32_t count = slots == 0U ? 1 : slots;
    const int32_t height = micropixel::math::Max((area.height - pad * (count + 1)) / count, 22);
    return {area.x + pad, area.y + pad + static_cast<int32_t>(slot) * (height + pad), area.width - pad * 2, height};
}

// Skill and item lists flow into two columns once they outgrow one.
micropixel::Rect ListRowRect(const GameContext& context, uint8_t row, uint8_t rows) {
    const micropixel::Rect area = context.layout.command;
    const int32_t pad = 4;
    const int32_t columns = rows > 2U ? 2 : 1;
    const int32_t per_column = micropixel::math::Max((static_cast<int32_t>(rows) + columns - 1) / columns, 1);
    const int32_t height = micropixel::math::Max((area.height - pad * 2) / per_column, 15);
    const int32_t width = (area.width - pad * 2) / columns;
    const int32_t column = static_cast<int32_t>(row) / per_column;
    const int32_t line = static_cast<int32_t>(row) % per_column;
    return {area.x + pad + column * width, area.y + pad + line * height, width - 2, height - 2};
}

void DrawPopups(GameContext& context, const uint8_t* enemy_units, uint8_t enemy_count,
                const uint8_t* party_units, uint8_t party_count) {
    GameView& view = context.view;
    for (uint8_t index = 0U; index < context.battle.unit_count; ++index) {
        if (context.battle.popup_ms[index] == 0U || context.battle.popup[index] == 0) {
            continue;
        }
        micropixel::Rect anchor{};
        bool found = false;
        for (uint8_t slot = 0U; slot < enemy_count; ++slot) {
            if (enemy_units[slot] == index) {
                anchor = EnemyRect(context, slot, enemy_count);
                found = true;
                break;
            }
        }
        for (uint8_t slot = 0U; !found && slot < party_count; ++slot) {
            if (party_units[slot] == index) {
                anchor = PartyRect(context, slot, party_count);
                found = true;
                break;
            }
        }
        if (!found) {
            continue;
        }
        Line text;
        const int16_t value = context.battle.popup[index];
        if (value < 0) {
            (void)text.Append("-");
            (void)text.AppendUint(static_cast<uint32_t>(-static_cast<int32_t>(value)));
        } else {
            (void)text.Append("+");
            (void)text.AppendUint(static_cast<uint32_t>(value));
        }
        const uint32_t age = kPopupMs - context.battle.popup_ms[index];
        const int32_t rise = static_cast<int32_t>(age / 40U);
        // Keep the number on screen: a rising popup would otherwise walk off the
        // top edge of the scene while it is still alive.
        const int32_t y = micropixel::math::Max(anchor.y - 12 - rise, context.layout.screen.y + 2);
        view.CenterText(anchor.center_x(), y, text.c_str(), value < 0 ? theme::kHp : theme::kJade,
                        micropixel::SystemFont::kMedium);
    }
}

void DrawCommandPanel(GameContext& context) {
    GameView& view = context.view;
    const GameLayout& layout = context.layout;
    const BattleState& battle = context.battle;

    switch (BattleCurrentMenu(battle)) {
        case BattleMenu::kRoot: {
            for (uint8_t row = 0U; row < 5U; ++row) {
                const micropixel::Rect rect = layout.commands[row];
                const bool focused = battle.cursor == row;
                const bool enabled = BattleMenuRowEnabled(battle, context.progress, row);
                view.Round(rect, focused ? theme::kPanelDeep : theme::kPanel,
                           focused ? theme::kAccent : theme::kEdge, 8U, focused ? 2U : 1U);
                view.CenterText(rect.center_x(), rect.center_y() - 7, context.strings.Get(RootLabel(row)),
                                enabled ? (focused ? theme::kHighlight : theme::kText) : theme::kDim,
                                micropixel::SystemFont::kSmall);
            }
            break;
        }
        case BattleMenu::kSkills: {
            const uint8_t rows = BattleMenuRows(battle, context.progress);
            for (uint8_t row = 0U; row < rows; ++row) {
                const micropixel::Rect rect = ListRowRect(context, row, rows);
                const bool focused = battle.cursor == row;
                const bool enabled = BattleMenuRowEnabled(battle, context.progress, row);
                view.Round(rect, focused ? theme::kPanelDeep : theme::kPanel,
                           focused ? theme::kAccent : theme::kEdge, 6U, focused ? 2U : 1U);
                const uint8_t skill_id = BattleMenuSkill(battle, row);
                const SkillDef& skill = Skill(skill_id);
                view.Text({rect.x + 6, rect.y + 2}, context.strings.Get(skill.name),
                          enabled ? (focused ? theme::kHighlight : theme::kText) : theme::kDim,
                          micropixel::SystemFont::kSmall);
                Line cost;
                (void)cost.AppendUint(skill.mp_cost);
                view.Text({rect.x + rect.width - 8 - 14, rect.y + 2}, cost.c_str(),
                          enabled ? theme::kMp : theme::kDim, micropixel::SystemFont::kSmall);
            }
            break;
        }
        case BattleMenu::kItems: {
            const uint8_t rows = BattleMenuRows(battle, context.progress);
            for (uint8_t row = 0U; row < rows; ++row) {
                const micropixel::Rect rect = ListRowRect(context, row, rows);
                const bool focused = battle.cursor == row;
                view.Round(rect, focused ? theme::kPanelDeep : theme::kPanel,
                           focused ? theme::kAccent : theme::kEdge, 6U, focused ? 2U : 1U);
                const uint8_t slot = BattleMenuItemSlot(battle, context.progress, row);
                if (slot == kEmptySlot || slot >= context.progress.bag_size) {
                    continue;
                }
                const BagSlot& bag = context.progress.bag[slot];
                view.Text({rect.x + 6, rect.y + 2}, context.strings.Get(Item(bag.item).name),
                          focused ? theme::kHighlight : theme::kText, micropixel::SystemFont::kSmall);
                Line amount;
                (void)amount.Append("x");
                (void)amount.AppendUint(bag.count);
                view.Text({rect.x + rect.width - 8 - 21, rect.y + 2}, amount.c_str(), theme::kMuted,
                          micropixel::SystemFont::kSmall);
            }
            break;
        }
        case BattleMenu::kNone:
            // Aiming hides the menu, so the panel explains what to do instead of
            // sitting empty — otherwise the screen reads as frozen.
            if (battle.phase == BattlePhase::kTargetEnemy || battle.phase == BattlePhase::kTargetAlly) {
                const bool ally = battle.phase == BattlePhase::kTargetAlly;
                Line hint;
                (void)hint.Append(context.strings.Get(ally ? ids::Id::kUiPickAlly : ids::Id::kUiPickFoe));
                if (battle.target < battle.unit_count) {
                    (void)hint.Append("  ");
                    (void)hint.Append(context.strings.Get(UnitNameId(battle.units[battle.target])));
                }
                view.CenterText(layout.command.center_x(), layout.command.y + 7, hint.c_str(), theme::kText,
                                micropixel::SystemFont::kSmall);
                view.CenterText(layout.command.center_x(), layout.command.y + layout.command.height - 20,
                                context.strings.Get(ids::Id::kUiAdvance), theme::kDim,
                                micropixel::SystemFont::kSmall);
            }
            break;
    }
}

}  // namespace

// A skill and an item announce themselves by the localized name the rules wrote
// into the message strip, so match the content tables instead of keeping a
// second list of message ids to sync by hand.
bool IsSkillName(ids::Id message) {
    for (uint8_t index = 0U; index < SkillCount(); ++index) {
        if (Skill(index).name == message) {
            return true;
        }
    }
    return false;
}

bool IsItemName(ids::Id message) {
    for (uint8_t index = 0U; index < ItemCount(); ++index) {
        if (Item(index).name == message) {
            return true;
        }
    }
    return false;
}

uint8_t AliveFoes(const BattleState& battle) {
    uint8_t count = 0U;
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        if (battle.units[index].enemy && battle.units[index].alive) {
            ++count;
        }
    }
    return count;
}

// One cue per change of the message strip, which is the channel the battle rules
// already use to announce every action they resolve.
void BattleAudioCue(GameContext& context, const BattleState& battle) {
    const ids::Id message = battle.message;
    if (IsSkillName(message)) {
        context.audio.PlaySfx(SfxId::kSkill);
        return;
    }
    if (IsItemName(message)) {
        context.audio.PlaySfx(SfxId::kItem);
        return;
    }
    switch (message) {
        case ids::Id::kUiAttack:
            // The strip names the unit that was hit, so the side that unit
            // stands on decides whether this was our blade or theirs.
            context.audio.PlaySfx(battle.message_unit < battle.unit_count &&
                                          !battle.units[battle.message_unit].enemy
                                      ? SfxId::kHurt
                                      : SfxId::kAttack);
            break;
        case ids::Id::kBattleHealNote:
        case ids::Id::kBattleReviveNote:
            context.audio.PlaySfx(SfxId::kHeal);
            break;
        case ids::Id::kBattleDefendNote:
            context.audio.PlaySfx(SfxId::kGuard);
            break;
        case ids::Id::kBattleFleeFail:
        case ids::Id::kUiEscaped:
            context.audio.PlaySfx(SfxId::kCancel);
            break;
        case ids::Id::kBattlePoisonTick:
            context.audio.PlaySfx(SfxId::kPoison);
            break;
        case ids::Id::kUiVictory:
            context.audio.PlaySfx(SfxId::kVictory);
            break;
        case ids::Id::kUiDefeat:
        case ids::Id::kUiGameOver:
            context.audio.PlaySfx(SfxId::kDefeat);
            break;
        default:
            break;
    }
}

void BattleSceneEnter(GameContext& context) {
    context.cursor = 0U;
    context.audio.PlaySfx(SfxId::kEncounter);
    context.dirty = true;
}

// Everything the battle screen draws that can change between two ticks.
uint32_t BattleSignature(const BattleState& battle) {
    uint32_t hash = static_cast<uint32_t>(battle.phase) | (static_cast<uint32_t>(battle.actor) << 8U) |
                    (static_cast<uint32_t>(battle.cursor) << 16U) | (static_cast<uint32_t>(battle.target) << 24U);
    hash = hash * 31U + battle.round;
    hash = hash * 31U + static_cast<uint32_t>(battle.message);
    hash = hash * 31U + battle.message_unit;
    hash = hash * 31U + battle.menu_rows;
    hash = hash * 31U + battle.unit_count;
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        const Combatant& unit = battle.units[index];
        hash = hash * 31U + unit.hp;
        hash = hash * 31U + unit.mp;
        hash = hash * 31U + (unit.alive ? 1U : 0U) + (unit.defending ? 2U : 0U) + (unit.poisoned ? 4U : 0U);
    }
    return hash;
}

void BattleSceneUpdate(GameContext& context, uint32_t delta_ms) {
    const uint32_t before = BattleSignature(context.battle);
    const ids::Id message_before = context.battle.message;
    const uint8_t foes_before = AliveFoes(context.battle);
    const uint8_t levels_before = context.battle.level_ups;
    BattleUpdate(context.battle, context.progress, delta_ms, context.rng);

    if (context.battle.message != message_before) {
        BattleAudioCue(context, context.battle);
    }
    if (AliveFoes(context.battle) < foes_before) {
        context.audio.PlaySfx(SfxId::kFoeDown);
    }
    if (context.battle.level_ups != levels_before) {
        context.audio.PlaySfx(SfxId::kLevelUp);
    }

    if (BattleFinished(context.battle)) {
        LogTrace(context, "battle-end", BattleWon(context.battle) ? 1 : 0,
                 BattleLost(context.battle) ? 1 : 0);
        if (BattleWon(context.battle)) {
            AdvanceScript(context);
            return;
        }
        if (BattleLost(context.battle)) {
            // The party wakes up at the last rest point and the same battle
            // replays, so the story can never hard-lock on a bad roll.
            const uint8_t battle_id = context.battle.battle_id;
            PartyRestore(context.progress);
            StoreProgress(context);
            PushBattle(context, battle_id);
            return;
        }
        // Fled: the encounter is dodged, not the chapter abandoned. The party
        // keeps the damage it took, falls back to the entrance of the area it
        // came from, and the same foes still hold the way out.
        SyncPartyFromBattle(context.battle, context.progress);
        StoreProgress(context);
        context.retreat_battle = context.battle.battle_id;
        WorldEnter(context.world, context.progress.map_id);
        PushScene(context, kSceneExplore);
        return;
    }
    // Floating damage numbers rise every tick; otherwise redraw only on change.
    bool animating = false;
    for (uint8_t index = 0U; index < context.battle.unit_count; ++index) {
        animating = animating || context.battle.popup_ms[index] > 0U;
    }
    if (animating || BattleSignature(context.battle) != before) {
        context.dirty = true;
    }
}

bool BattleSceneTouch(GameContext& context, const micropixel::TouchEvent& touch) {
    if (touch.phase() != micropixel::TouchPhase::kDown) {
        return false;
    }
    const BattleState& battle = context.battle;
    const micropixel::Point point = touch.position();

    switch (BattleCurrentMenu(battle)) {
        case BattleMenu::kRoot:
            for (uint8_t row = 0U; row < 5U; ++row) {
                if (!context.layout.commands[row].contains(point)) {
                    continue;
                }
                if (battle.cursor != row) {
                    (void)BattleMoveCursor(context.battle, static_cast<int32_t>(row) - static_cast<int32_t>(battle.cursor));
                }
                (void)BattleConfirm(context.battle, context.progress, context.rng);
                context.dirty = true;
                return true;
            }
            break;
        case BattleMenu::kSkills:
        case BattleMenu::kItems: {
            const uint8_t rows = BattleMenuRows(battle, context.progress);
            for (uint8_t row = 0U; row < rows; ++row) {
                if (!ListRowRect(context, row, rows).contains(point)) {
                    continue;
                }
                if (battle.cursor != row) {
                    (void)BattleMoveCursor(context.battle, static_cast<int32_t>(row) - static_cast<int32_t>(battle.cursor));
                }
                (void)BattleConfirm(context.battle, context.progress, context.rng);
                context.dirty = true;
                return true;
            }
            break;
        }
        default:
            break;
    }

    // Targeting: tap the unit directly.
    if (battle.phase == BattlePhase::kTargetEnemy || battle.phase == BattlePhase::kTargetAlly) {
        uint8_t units[kMaxCombatants]{};
        const uint8_t enemies = CollectEnemies(context, units);
        for (uint8_t slot = 0U; battle.phase == BattlePhase::kTargetEnemy && slot < enemies; ++slot) {
            // A fallen foe is not a target: tapping it must not burn the turn.
            if (EnemyRect(context, slot, enemies).contains(point) && BattleUnit(battle, units[slot]).alive) {
                context.battle.target = units[slot];
                (void)BattleConfirm(context.battle, context.progress, context.rng);
                context.dirty = true;
                return true;
            }
        }
        uint8_t party[kMaxCombatants]{};
        const uint8_t members = CollectParty(context, party);
        for (uint8_t slot = 0U; battle.phase == BattlePhase::kTargetAlly && slot < members; ++slot) {
            if (!PartyRect(context, slot, members).contains(point)) {
                continue;
            }
            const Combatant& unit = BattleUnit(context.battle, party[slot]);
            const bool allowed = battle.target_dead ? !unit.alive : unit.alive;
            if (!allowed) {
                continue;
            }
            context.battle.target = party[slot];
            (void)BattleConfirm(context.battle, context.progress, context.rng);
            context.dirty = true;
            return true;
        }
        // The command panel is empty while aiming, so a tap there backs out.
        if (context.layout.command.contains(point) || context.layout.message.contains(point)) {
            (void)BattleBack(context.battle);
            context.dirty = true;
        }
        return true;
    }

    // A tap beside an open list backs out of it; beside the root menu it does nothing.
    if (battle.phase == BattlePhase::kSkillMenu || battle.phase == BattlePhase::kItemMenu) {
        (void)BattleBack(context.battle);
        context.dirty = true;
        return true;
    }
    if (battle.phase == BattlePhase::kCommand) {
        return true;
    }

    // Anywhere else: advance the splash, the resolve hold or the result screen.
    (void)BattleConfirm(context.battle, context.progress, context.rng);
    context.dirty = true;
    return true;
}

// Cursor step for a direction key in the current menu's on-screen layout: the
// root menu is a 3-column grid of 5, lists flow column-major into two columns,
// and target selection just cycles.
int32_t KeyStep(const BattleState& battle, micropixel::KeyCode code) {
    const bool horizontal = code == micropixel::KeyCode::kLeft || code == micropixel::KeyCode::kRight;
    const int32_t sign = (code == micropixel::KeyCode::kLeft || code == micropixel::KeyCode::kUp) ? -1 : 1;
    const int32_t cursor = battle.cursor;
    switch (battle.phase) {
        case BattlePhase::kCommand:
            if (horizontal) {
                return sign;
            }
            if (sign < 0) {
                return cursor >= 3 ? -3 : 0;
            }
            return cursor < 3 ? micropixel::math::Min(3, 4 - cursor) : 0;
        case BattlePhase::kSkillMenu:
        case BattlePhase::kItemMenu: {
            const int32_t rows = battle.menu_rows;
            const int32_t columns = rows > 2 ? 2 : 1;
            const int32_t per_column = micropixel::math::Max((rows + columns - 1) / columns, 1);
            if (horizontal) {
                const int32_t next = cursor + sign * per_column;
                return next >= 0 && next < rows ? sign * per_column : 0;
            }
            const int32_t next = cursor + sign;
            return next >= 0 && next < rows && next / per_column == cursor / per_column ? sign : 0;
        }
        default:
            return sign;
    }
}

bool BattleSceneKey(GameContext& context, micropixel::KeyCode code) {
    switch (code) {
        case micropixel::KeyCode::kLeft:
        case micropixel::KeyCode::kRight:
        case micropixel::KeyCode::kUp:
        case micropixel::KeyCode::kDown: {
            const int32_t step = KeyStep(context.battle, code);
            return step != 0 && BattleMoveCursor(context.battle, step);
        }
        case micropixel::KeyCode::kConfirm:
        case micropixel::KeyCode::kSouth:
            return BattleConfirm(context.battle, context.progress, context.rng);
        case micropixel::KeyCode::kBack: {
            if (BattleCancellable(context.battle)) {
                return BattleBack(context.battle);
            }
            return false;
        }
        default:
            return false;
    }
}

void BattleSceneRender(GameContext& context) {
    GameView& view = context.view;
    const GameLayout& layout = context.layout;
    const BattleState& battle = context.battle;
    view.Begin();

    view.Fill(layout.screen, theme::kNight);

    uint8_t enemies[kMaxCombatants]{};
    const uint8_t enemy_count = CollectEnemies(context, enemies);
    uint8_t party[kMaxCombatants]{};
    const uint8_t party_count = CollectParty(context, party);

    // Round counter in the header.
    Line round_text;
    (void)round_text.Append(context.strings.Get(ids::Id::kUiRoundPrefix));
    (void)round_text.AppendUint(battle.round);
    (void)round_text.Append(context.strings.Get(ids::Id::kUiRoundSuffix));
    view.Round(layout.header, theme::kPanelDeep, theme::kEdge, 6U, 1U);
    view.Text({layout.header.x + 8, layout.header.y + 3}, round_text.c_str(), theme::kAccent,
              micropixel::SystemFont::kSmall);

    // Foes: one framed plate each, the art inside it, the name as a caption on
    // the plate and the foe's HP along its foot. Everything is kept inside the
    // plate so nothing can spill out of the enemy band.
    for (uint8_t slot = 0U; slot < enemy_count; ++slot) {
        const uint8_t unit_index = enemies[slot];
        const Combatant& unit = BattleUnit(battle, unit_index);
        const micropixel::Rect rect = EnemyRect(context, slot, enemy_count);
        const bool targeted = (battle.phase == BattlePhase::kTargetEnemy) && battle.target == unit_index;
        view.Round(rect, theme::kPanelDeep,
                   targeted ? theme::kAccent : (unit.alive ? theme::kBlood : theme::kDim), 6U,
                   targeted ? 2U : 1U);
        widgets::EnemySprite(view, EnemyArt(rect), Enemy(unit.species).sprite, unit.alive, false);
        view.CenterText(rect.center_x(), rect.y + 2, context.strings.Get(UnitNameId(unit)),
                        unit.alive ? theme::kText : theme::kDim, micropixel::SystemFont::kSmall);
        if (unit.alive) {
            view.Bar({rect.x + 6, rect.y + rect.height - 9, rect.width - 12, 5}, unit.hp, unit.max_hp, theme::kHp);
        }
    }

    // Party cards: one row each — name, level, HP/MP bars, HP figures. Status
    // (poison, guard) is shown by the card's border to keep the row uncluttered.
    for (uint8_t slot = 0U; slot < party_count; ++slot) {
        const uint8_t unit_index = party[slot];
        const Combatant& unit = BattleUnit(battle, unit_index);
        const micropixel::Rect rect = PartyRect(context, slot, party_count);
        const bool is_actor = battle.actor == unit_index &&
                              (battle.phase == BattlePhase::kCommand || battle.phase == BattlePhase::kSkillMenu ||
                               battle.phase == BattlePhase::kItemMenu);
        const bool targeted = (battle.phase == BattlePhase::kTargetAlly) && battle.target == unit_index;
        micropixel::Color edge = is_actor ? theme::kAccent : theme::kEdge;
        if (unit.poisoned) {
            edge = theme::kJade;
        } else if (unit.defending) {
            edge = theme::kMp;
        }
        widgets::Panel(view, rect, is_actor ? theme::kPanel : theme::kPanelDeep, targeted ? theme::kHighlight : edge,
                       8U);
        const int32_t text_y = rect.y + (rect.height - 18) / 2;
        view.Text({rect.x + 8, text_y}, context.strings.Get(UnitNameId(unit)),
                  unit.alive ? theme::kText : theme::kDim, micropixel::SystemFont::kSmall);
        Line level;
        (void)level.Append(context.strings.Get(ids::Id::kUiLv));
        (void)level.AppendUint(unit.level);
        view.Text({rect.x + 64, text_y}, level.c_str(), theme::kAccent, micropixel::SystemFont::kSmall);

        const int32_t bar_x = rect.x + 112;
        const int32_t bar_w = micropixel::math::Max(rect.width - 112 - 64, 30);
        view.Bar({bar_x, rect.y + rect.height / 2 - 7, bar_w, 6}, unit.hp, unit.max_hp, theme::kHp);
        view.Bar({bar_x, rect.y + rect.height / 2 + 1, bar_w, 6}, unit.mp, unit.max_mp, theme::kMp);
        Line hp_text;
        (void)hp_text.AppendUint(unit.hp);
        (void)hp_text.Append("/");
        (void)hp_text.AppendUint(unit.max_hp);
        int32_t hp_width = 0;
        for (const char* cursor = hp_text.c_str(); *cursor != '\0'; ++cursor) {
            hp_width += 7;  // digits and '/' in the small font
        }
        view.Text({rect.x + rect.width - 8 - hp_width, text_y}, hp_text.c_str(), theme::kMuted,
                  micropixel::SystemFont::kSmall);
    }

    // Message strip: the acting hero's name while a menu is open, otherwise the
    // outcome of the action that just resolved.
    widgets::Panel(view, layout.message, theme::kPanelDeep, theme::kEdge, 8U);
    const BattlePhase phase = battle.phase;
    const bool choosing = phase == BattlePhase::kCommand || phase == BattlePhase::kSkillMenu ||
                          phase == BattlePhase::kItemMenu || phase == BattlePhase::kTargetEnemy ||
                          phase == BattlePhase::kTargetAlly;
    Line message;
    if (choosing && battle.unit_count > 0U) {
        (void)message.Append(context.strings.Get(UnitNameId(BattleActor(battle))));
    } else {
        (void)message.Append(context.strings.Get(battle.message));
        if (battle.message_unit != kEmptySlot && battle.message_unit < battle.unit_count) {
            (void)message.Append("  ");
            (void)message.Append(context.strings.Get(UnitNameId(BattleUnit(battle, battle.message_unit))));
        }
    }
    const int32_t message_y = layout.message.y + (layout.message.height - 18) / 2;
    view.CenterText(layout.message.center_x(), message_y, message.c_str(), theme::kText,
                    micropixel::SystemFont::kSmall);

    if (phase == BattlePhase::kVictory) {
        Line reward;
        (void)reward.Append(context.strings.Get(ids::Id::kBattleGainXp));
        (void)reward.AppendUint(battle.total_xp);
        (void)reward.Append("   ");
        (void)reward.Append(context.strings.Get(ids::Id::kBattleGainGold));
        (void)reward.AppendUint(battle.total_gold);
        if (battle.level_ups > 0U) {
            (void)reward.Append("   ");
            (void)reward.Append(context.strings.Get(ids::Id::kBattleLevelTo));
            (void)reward.Append("+");
            (void)reward.AppendUint(battle.level_ups);
        }
        view.CenterText(layout.enemy_area.center_x(), layout.enemy_area.y + layout.enemy_area.height / 3,
                        reward.c_str(), theme::kAccent, micropixel::SystemFont::kMedium);
    }
    if (phase == BattlePhase::kVictory || phase == BattlePhase::kDefeat || phase == BattlePhase::kFled) {
        view.Text({layout.message.x + layout.message.width - 10 - 52, message_y},
                  context.strings.Get(ids::Id::kUiAdvance), theme::kDim, micropixel::SystemFont::kSmall);
    }

    DrawCommandPanel(context);
    DrawPopups(context, enemies, enemy_count, party, party_count);
    view.End();
}

}  // namespace pal
