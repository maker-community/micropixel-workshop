// SPDX-License-Identifier: Apache-2.0
// Battle scene: foes, party cards, the message strip and the command panel.

#include "../ashes_common.hpp"
#include "../ashes_widgets.hpp"

#include "sdk/math.hpp"

namespace ashes {
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
    return unit.enemy ? Enemy(unit.species).name : PortraitNameId(unit.species);
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
    const int32_t width = micropixel::math::Max(area.width / (slots == 0U ? 1 : slots), 24);
    const int32_t size = micropixel::math::Max(
        micropixel::math::Min(width - 10, area.height - 22), 20);
    return {area.x + static_cast<int32_t>(slot) * width + (width - size) / 2,
            area.y + area.height - size - 14, size, size};
}

micropixel::Rect PartyRect(const GameContext& context, uint8_t slot, uint8_t slots) {
    const micropixel::Rect area = context.layout.party_area;
    const int32_t pad = 5;
    const int32_t count = slots == 0U ? 1 : slots;
    const int32_t height = micropixel::math::Max((area.height - pad * (count + 1)) / count, 24);
    return {area.x + pad, area.y + pad + static_cast<int32_t>(slot) * (height + pad), area.width - pad * 2, height};
}

micropixel::Rect ListRowRect(const GameContext& context, uint8_t row, uint8_t rows) {
    const micropixel::Rect area = context.layout.command;
    const int32_t pad = 4;
    const int32_t count = micropixel::math::Max<int32_t>(rows, 1);
    const int32_t height = micropixel::math::Max((area.height - pad * 2) / count, 15);
    return {area.x + pad, area.y + pad + static_cast<int32_t>(row) * height, area.width - pad * 2, height - 2};
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
                view.Text({rect.x + rect.width - 6, rect.y + 2}, cost.c_str(),
                          enabled ? theme::kMp : theme::kDim, micropixel::SystemFont::kSmall, true);
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
                view.Text({rect.x + rect.width - 6, rect.y + 2}, amount.c_str(), theme::kMuted,
                          micropixel::SystemFont::kSmall, true);
            }
            break;
        }
        case BattleMenu::kNone:
            break;
    }
}

}  // namespace

void BattleSceneEnter(GameContext& context) {
    context.cursor = 0U;
    context.dirty = true;
}

void BattleSceneUpdate(GameContext& context, uint32_t delta_ms) {
    BattleUpdate(context.battle, context.progress, delta_ms, context.rng);

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
        // Fled: the chapter is abandoned. "继续前缘" replays it from the save.
        PartyRestore(context.progress);
        StoreProgress(context);
        PushScene(context, kSceneTitle);
        return;
    }
    context.dirty = true;
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
        for (uint8_t slot = 0U; slot < enemies; ++slot) {
            if (EnemyRect(context, slot, enemies).contains(point)) {
                context.battle.target = units[slot];
                (void)BattleConfirm(context.battle, context.progress, context.rng);
                context.dirty = true;
                return true;
            }
        }
        uint8_t party[kMaxCombatants]{};
        const uint8_t members = CollectParty(context, party);
        for (uint8_t slot = 0U; slot < members; ++slot) {
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
        return true;
    }

    // Anywhere else: advance the splash, the resolve hold or the result screen.
    (void)BattleConfirm(context.battle, context.progress, context.rng);
    context.dirty = true;
    return true;
}

bool BattleSceneKey(GameContext& context, micropixel::KeyCode code) {
    switch (code) {
        case micropixel::KeyCode::kLeft:
            return BattleMoveCursor(context.battle, -1);
        case micropixel::KeyCode::kRight:
            return BattleMoveCursor(context.battle, 1);
        case micropixel::KeyCode::kUp:
            return BattleMoveCursor(context.battle, context.battle.phase == BattlePhase::kCommand ? -3 : -1);
        case micropixel::KeyCode::kDown:
            return BattleMoveCursor(context.battle, context.battle.phase == BattlePhase::kCommand ? 3 : 1);
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
        widgets::EnemySprite(view, rect.inset(6), Enemy(unit.species).sprite, unit.alive, false);
        view.Text({rect.x + 5, rect.y + 2}, context.strings.Get(UnitNameId(unit)),
                  unit.alive ? theme::kText : theme::kDim, micropixel::SystemFont::kSmall);
        if (unit.alive) {
            view.Bar({rect.x + 5, rect.y + rect.height - 8, rect.width - 10, 5}, unit.hp, unit.max_hp, theme::kHp);
        }
    }

    // Party cards.
    for (uint8_t slot = 0U; slot < party_count; ++slot) {
        const uint8_t unit_index = party[slot];
        const Combatant& unit = BattleUnit(battle, unit_index);
        const micropixel::Rect rect = PartyRect(context, slot, party_count);
        const bool is_actor = battle.actor == unit_index &&
                              (battle.phase == BattlePhase::kCommand || battle.phase == BattlePhase::kSkillMenu ||
                               battle.phase == BattlePhase::kItemMenu);
        const bool targeted = (battle.phase == BattlePhase::kTargetAlly) && battle.target == unit_index;
        widgets::Panel(view, rect, is_actor ? theme::kPanel : theme::kPanelDeep,
                       targeted ? theme::kHighlight : (is_actor ? theme::kAccent : theme::kEdge), 8U);
        view.Text({rect.x + 6, rect.y + 2}, context.strings.Get(UnitNameId(unit)),
                  unit.alive ? theme::kText : theme::kDim, micropixel::SystemFont::kSmall);
        Line level;
        (void)level.Append(context.strings.Get(ids::Id::kUiLv));
        (void)level.AppendUint(unit.level);
        view.Text({rect.x + rect.width - 6, rect.y + 2}, level.c_str(), theme::kAccent,
                  micropixel::SystemFont::kSmall, true);

        const micropixel::Rect bars{rect.x + 6, rect.y + rect.height / 2, rect.width - 12,
                                    micropixel::math::Max(rect.height / 2 - 6, 12)};
        widgets::HpMpBars(view, bars, unit, true);
        Line hp_text;
        (void)hp_text.AppendUint(unit.hp);
        (void)hp_text.Append("/");
        (void)hp_text.AppendUint(unit.max_hp);
        view.Text({rect.x + 6, rect.y + rect.height - 14}, hp_text.c_str(), theme::kMuted,
                  micropixel::SystemFont::kSmall);
        if (unit.poisoned) {
            view.Text({rect.x + rect.width / 2, rect.y + rect.height - 14}, context.strings.Get(ids::Id::kBattlePoisonTick),
                      theme::kJade, micropixel::SystemFont::kSmall, true);
        }
        if (unit.defending) {
            view.Text({rect.x + rect.width - 6, rect.y + rect.height - 14},
                      context.strings.Get(ids::Id::kUiDefend), theme::kMp, micropixel::SystemFont::kSmall, true);
        }
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
    view.CenterText(layout.message.center_x(), layout.message.y + 6, message.c_str(), theme::kText,
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
        view.CenterText(layout.message.center_x(), layout.message.y + layout.message.height - 17,
                        context.strings.Get(ids::Id::kUiAdvance), theme::kDim, micropixel::SystemFont::kSmall);
    }

    DrawCommandPanel(context);
    DrawPopups(context, enemies, enemy_count, party, party_count);
    view.End();
}

}  // namespace ashes
