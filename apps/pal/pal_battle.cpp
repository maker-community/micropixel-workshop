// SPDX-License-Identifier: MIT
// Turn-based combat rules, turn order, enemy AI and rewards.

#include "pal_battle.hpp"

#include "sdk/math.hpp"

namespace pal {

// Writes the fight's HP/MP back into the party. Winning, losing and fleeing all
// settle the battle through this, so a retreat keeps the damage it took instead
// of quietly restoring everyone to full.
void SyncPartyFromBattle(const BattleState& battle, Progress& progress) {
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        const Combatant& unit = battle.units[index];
        if (unit.enemy || unit.slot >= progress.party_size || unit.slot >= kMaxParty) {
            continue;
        }
        PartyMember& member = progress.party[unit.slot];
        if (member.character != unit.species) {
            continue;
        }
        member.hp = unit.hp;
        member.mp = unit.mp;
        member.poisoned = unit.poisoned;
    }
}

namespace {

inline constexpr uint32_t kIntroMs = 1200U;
inline constexpr uint32_t kResolveMs = 780U;

// --- target helpers ---------------------------------------------------------

uint8_t FirstLivingEnemy(const BattleState& battle) {
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        if (battle.units[index].enemy && battle.units[index].alive) {
            return index;
        }
    }
    return kEmptySlot;
}

uint8_t FirstLivingAlly(const BattleState& battle) {
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        if (!battle.units[index].enemy && battle.units[index].alive) {
            return index;
        }
    }
    return kEmptySlot;
}

uint8_t FirstFallenAlly(const BattleState& battle) {
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        if (!battle.units[index].enemy && !battle.units[index].alive) {
            return index;
        }
    }
    return kEmptySlot;
}

uint8_t CycleTarget(const BattleState& battle, uint8_t from, int32_t delta, bool enemy_side, bool want_dead) {
    const uint8_t count = battle.unit_count;
    if (count == 0U) {
        return kEmptySlot;
    }
    int32_t index = from < count ? static_cast<int32_t>(from) : 0;
    for (uint8_t step = 0U; step < count; ++step) {
        index = (index + delta + count) % count;
        const Combatant& unit = battle.units[index];
        if (unit.enemy != enemy_side) {
            continue;
        }
        if (want_dead ? !unit.alive : unit.alive) {
            return static_cast<uint8_t>(index);
        }
    }
    return from < count ? from : kEmptySlot;
}

uint8_t RandomLivingAlly(const BattleState& battle, micropixel::XorShift32& rng) {
    // Heroes only: this is what a foe picks when it wants someone to hit.
    uint8_t candidates[kMaxCombatants]{};
    uint8_t count = 0U;
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        if (!battle.units[index].enemy && battle.units[index].alive) {
            candidates[count++] = index;
        }
    }
    if (count == 0U) {
        return kEmptySlot;
    }
    return candidates[rng.Below(count)];
}

// The most wounded living unit on one side, or kEmptySlot when nobody is hurt
// below `percent` of their maximum.
uint8_t MostHurtOnSide(const BattleState& battle, bool enemy_side, uint32_t percent) {
    uint8_t best = kEmptySlot;
    uint32_t best_ratio = percent;
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        const Combatant& unit = battle.units[index];
        if (unit.enemy != enemy_side || !unit.alive || unit.max_hp == 0U) {
            continue;
        }
        const uint32_t ratio = static_cast<uint32_t>(unit.hp) * 100U / unit.max_hp;
        if (ratio < best_ratio) {
            best_ratio = ratio;
            best = index;
        }
    }
    return best;
}

// --- damage -----------------------------------------------------------------

int32_t EffectiveDefense(const Combatant& unit) {
    int32_t value = unit.def;
    value += value * unit.def_mod / 100;
    return value < 0 ? 0 : value;
}

int32_t Vary(micropixel::XorShift32& rng, int32_t value, uint32_t percent) {
    const int32_t span = value * static_cast<int32_t>(percent) / 100;
    if (span <= 0) {
        return value;
    }
    return value - span + static_cast<int32_t>(rng.Below(static_cast<uint32_t>(span) * 2U + 1U));
}

void SetPopup(BattleState& battle, uint8_t unit, int32_t value) {
    battle.popup[unit] = static_cast<int16_t>(micropixel::math::Clamp<int32_t>(value, -32000, 32000));
    battle.popup_ms[unit] = kPopupMs;
}

int32_t DealDamage(BattleState& battle, uint8_t unit_index, int32_t raw) {
    Combatant& target = battle.units[unit_index];
    int32_t amount = raw;
    if (target.defending) {
        amount /= 2;
    }
    if (amount < 1) {
        amount = 1;
    }
    if (static_cast<int32_t>(target.hp) <= amount) {
        amount = static_cast<int32_t>(target.hp);
        target.hp = 0U;
        target.alive = false;
    } else {
        target.hp = static_cast<uint16_t>(target.hp - amount);
    }
    SetPopup(battle, unit_index, -amount);
    return amount;
}

void RestoreHealth(BattleState& battle, uint8_t unit_index, int32_t amount) {
    Combatant& target = battle.units[unit_index];
    if (!target.alive) {
        return;
    }
    const int32_t room = static_cast<int32_t>(target.max_hp) - static_cast<int32_t>(target.hp);
    if (amount > room) {
        amount = room;
    }
    if (amount <= 0) {
        return;
    }
    target.hp = static_cast<uint16_t>(target.hp + amount);
    SetPopup(battle, unit_index, amount);
}

// --- turn order -------------------------------------------------------------

void BuildOrder(BattleState& battle) {
    uint8_t count = 0U;
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        if (battle.units[index].alive) {
            battle.order[count++] = index;
        }
    }
    // Insertion sort, fastest first; ties keep the earlier unit ahead.
    for (uint8_t index = 1U; index < count; ++index) {
        const uint8_t key = battle.order[index];
        int32_t probe = static_cast<int32_t>(index) - 1;
        while (probe >= 0 && battle.units[battle.order[probe]].spd < battle.units[key].spd) {
            battle.order[probe + 1] = battle.order[probe];
            --probe;
        }
        battle.order[probe + 1] = key;
    }
    battle.order_count = count;
}

// --- actions ----------------------------------------------------------------

void PerformBasicAttack(BattleState& battle, micropixel::XorShift32& rng, uint8_t target_index) {
    if (target_index == kEmptySlot) {
        return;
    }
    const Combatant& attacker = battle.units[battle.actor];
    const int32_t base = static_cast<int32_t>(attacker.atk) * 130 / 100 - EffectiveDefense(battle.units[target_index]) / 2;
    (void)DealDamage(battle, target_index, Vary(rng, base < 1 ? 1 : base, 12U));
    battle.message = ids::Id::kUiAttack;
    battle.message_unit = target_index;
    battle.phase = BattlePhase::kResolve;
    battle.wait_ms = kResolveMs;
}

uint8_t CollectTargets(const BattleState& battle, TargetSide side, uint8_t primary, uint8_t* out) {
    uint8_t count = 0U;
    // "Enemy" and "ally" in a skill are relative to whoever casts it: a foe's
    // area spell must land on the party, not on the foe's own side.
    const bool caster_is_foe = battle.units[battle.actor].enemy;
    const auto collect_side = [&](bool want_foes) {
        for (uint8_t index = 0U; index < battle.unit_count; ++index) {
            if (battle.units[index].enemy == want_foes && battle.units[index].alive) {
                out[count++] = index;
            }
        }
    };
    switch (side) {
        case TargetSide::kOneEnemy:
        case TargetSide::kOneAlly:
            if (primary != kEmptySlot && primary < battle.unit_count) {
                out[count++] = primary;
            }
            break;
        case TargetSide::kAllEnemies:
            collect_side(!caster_is_foe);
            break;
        case TargetSide::kAllAllies:
            collect_side(caster_is_foe);
            break;
        case TargetSide::kSelf:
            out[count++] = battle.actor;
            break;
    }
    return count;
}

// Scales a computed hit by how much the victim cares about that element. A
// resisted hit still registers for at least 1.
int32_t ElementAdjusted(const BattleState& battle, uint8_t victim, uint8_t skill_id, int32_t amount) {
    const uint8_t element = SkillElement(skill_id);
    if (element == kElemNone) {
        return amount;
    }
    const Combatant& unit = battle.units[victim];
    const int16_t resist = unit.enemy ? EnemyElementResist(unit.species, element)
                                      : CharacterElementResist(unit.species, element);
    return micropixel::math::Max<int32_t>(amount * (100 + resist) / 100, 1);
}

// A won fight hands back part of what it cost: the harder it was measured
// against the party's next level, the more it gives back. Deliberately never a
// full heal - that is what 金创药 and 灵葫仙丹 are for.
void RestAfterBattle(Progress& progress, uint32_t xp) {
    if (xp == 0U) {
        return;
    }
    for (uint8_t index = 0U; index < progress.party_size && index < kMaxParty; ++index) {
        PartyMember& member = progress.party[index];
        if (member.character == kEmptySlot || member.hp == 0U) {
            continue;
        }
        const uint8_t next = member.level < kMaxLevel ? static_cast<uint8_t>(member.level + 1U) : kMaxLevel;
        const uint32_t need = static_cast<uint32_t>(MemberXpThreshold(next)) - MemberXpThreshold(member.level);
        uint32_t share = need / 5U / xp;
        if (share < 2U) {
            share = 2U;
        }
        member.hp = static_cast<uint16_t>(member.hp + (MemberMaxHp(member) - member.hp) / share);
        member.mp = static_cast<uint16_t>(member.mp + (MemberMaxMp(member) - member.mp) / share);
    }
}

void PerformSkill(BattleState& battle, micropixel::XorShift32& rng, uint8_t skill_id, uint8_t target_index) {
    const SkillDef& skill = Skill(skill_id);
    Combatant& caster = battle.units[battle.actor];
    if (caster.mp < skill.mp_cost) {
        return;
    }
    caster.mp = static_cast<uint16_t>(caster.mp - skill.mp_cost);
    battle.pending_skill = skill_id;

    uint8_t targets[kMaxCombatants]{};
    const uint8_t count = CollectTargets(battle, skill.target, target_index, targets);
    if (count == 0U) {
        return;
    }

    for (uint8_t index = 0U; index < count; ++index) {
        const uint8_t victim = targets[index];
        switch (skill.kind) {
            case SkillKind::kPhysical: {
                const int32_t base = static_cast<int32_t>(caster.atk) * skill.power / 100 -
                                     EffectiveDefense(battle.units[victim]) / 2;
                (void)DealDamage(battle, victim, Vary(rng, base < 1 ? 1 : base, 10U));
                break;
            }
            case SkillKind::kMagic: {
                const int32_t base = static_cast<int32_t>(caster.atk) * skill.power / 100 +
                                     static_cast<int32_t>(caster.level) * 2 -
                                     EffectiveDefense(battle.units[victim]) / 3;
                (void)DealDamage(battle, victim,
                                 ElementAdjusted(battle, victim, skill_id, Vary(rng, base < 1 ? 1 : base, 8U)));
                break;
            }
            case SkillKind::kHeal:
                RestoreHealth(battle, victim, static_cast<int32_t>(skill.power) + caster.level * 3);
                break;
            case SkillKind::kDrain: {
                const int32_t base = static_cast<int32_t>(caster.atk) * skill.power / 100 -
                                     EffectiveDefense(battle.units[victim]) / 2;
                const int32_t dealt = DealDamage(battle, victim, Vary(rng, base < 1 ? 1 : base, 10U));
                RestoreHealth(battle, battle.actor, dealt / 2);
                break;
            }
            case SkillKind::kBind:
                battle.units[victim].def_mod = -40;
                battle.units[victim].def_mod_rounds = 3U;
                break;
            case SkillKind::kPoison: {
                const int32_t base = static_cast<int32_t>(caster.atk) * skill.power / 100 +
                                     static_cast<int32_t>(caster.level) * 2 -
                                     EffectiveDefense(battle.units[victim]) / 3;
                (void)DealDamage(battle, victim,
                                 ElementAdjusted(battle, victim, skill_id, Vary(rng, base < 1 ? 1 : base, 8U)));
                if (battle.units[victim].alive) {
                    battle.units[victim].poisoned = true;
                }
                break;
            }
            case SkillKind::kGuard:
                battle.units[victim].defending = true;
                break;
        }
    }

    switch (skill.kind) {
        case SkillKind::kHeal:
            battle.message = ids::Id::kBattleHealNote;
            break;
        case SkillKind::kGuard:
            battle.message = ids::Id::kBattleDefendNote;
            break;
        default:
            battle.message = skill.name;
            break;
    }
    battle.message_unit = target_index < battle.unit_count ? target_index : kEmptySlot;
    battle.phase = BattlePhase::kResolve;
    battle.wait_ms = kResolveMs;
}

void PerformItem(BattleState& battle, Progress& progress, uint8_t bag_slot, uint8_t target_index) {
    if (bag_slot >= progress.bag_size) {
        return;
    }
    const uint8_t item_id = progress.bag[bag_slot].item;
    const ItemDef& item = Item(item_id);
    if (item_id == kItemNone || !item.battle_usable) {
        return;
    }

    switch (item.effect) {
        case ItemEffect::kHealHp:
            if (target_index != kEmptySlot) {
                RestoreHealth(battle, target_index, item.magnitude);
            }
            battle.message = ids::Id::kBattleHealNote;
            break;
        case ItemEffect::kHealMp:
            if (target_index != kEmptySlot && battle.units[target_index].alive) {
                Combatant& target = battle.units[target_index];
                const int32_t room = static_cast<int32_t>(target.max_mp) - static_cast<int32_t>(target.mp);
                int32_t amount = item.magnitude < room ? item.magnitude : room;
                if (amount < 0) {
                    amount = 0;
                }
                target.mp = static_cast<uint16_t>(target.mp + amount);
                SetPopup(battle, target_index, amount);
            }
            battle.message = ids::Id::kBattleHealNote;
            break;
        case ItemEffect::kCurePoison:
            if (target_index != kEmptySlot) {
                battle.units[target_index].poisoned = false;
            }
            battle.message = ids::Id::kBattleReviveNote;
            break;
        case ItemEffect::kDamageAll:
            for (uint8_t index = 0U; index < battle.unit_count; ++index) {
                if (battle.units[index].enemy && battle.units[index].alive) {
                    (void)DealDamage(battle, index, item.magnitude);
                }
            }
            battle.message = item.name;
            break;
        case ItemEffect::kRevive:
            if (target_index != kEmptySlot && !battle.units[target_index].alive) {
                Combatant& target = battle.units[target_index];
                target.alive = true;
                target.hp = item.magnitude < target.max_hp ? item.magnitude : target.max_hp;
                SetPopup(battle, target_index, target.hp);
            }
            battle.message = ids::Id::kBattleReviveNote;
            break;
        case ItemEffect::kKey:
            return;
    }

    (void)BagConsume(progress, item_id, 1U);
    battle.message_unit = target_index < battle.unit_count ? target_index : kEmptySlot;
    battle.phase = BattlePhase::kResolve;
    battle.wait_ms = kResolveMs;
}

void PerformDefend(BattleState& battle) {
    battle.units[battle.actor].defending = true;
    battle.message = ids::Id::kBattleDefendNote;
    battle.message_unit = battle.actor;
    battle.phase = BattlePhase::kResolve;
    battle.wait_ms = kResolveMs;
}

void TryFlee(BattleState& battle, micropixel::XorShift32& rng) {
    // A contest rather than a coin flip: how good the runner is at escaping
    // against how much the foes can do about it. Boss battles are closed
    // outright by `allow_flee`.
    uint32_t resistance = 0U;
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        const Combatant& unit = battle.units[index];
        if (unit.enemy && unit.alive) {
            resistance += static_cast<uint32_t>(unit.spd) * 2U + static_cast<uint32_t>(unit.atk) / 2U;
        }
    }
    const Combatant& runner = battle.units[battle.actor];
    const uint32_t escape =
        static_cast<uint32_t>(runner.spd) * 3U + static_cast<uint32_t>(runner.level) * 2U;
    if (battle.allow_flee && escape >= rng.Below(resistance + 1U)) {
        battle.outcome = BattleOutcome::kFled;
        battle.phase = BattlePhase::kFled;
        battle.message = ids::Id::kUiEscaped;
        battle.message_unit = kEmptySlot;
        battle.wait_ms = 0U;
        return;
    }
    // A failed escape burns the turn, like every classic of the genre.
    battle.message = ids::Id::kBattleFleeFail;
    battle.message_unit = battle.actor;
    battle.phase = BattlePhase::kResolve;
    battle.wait_ms = kResolveMs;
}

// --- enemy AI ---------------------------------------------------------------

void BeginEnemyAction(BattleState& battle, micropixel::XorShift32& rng) {
    Combatant& self = battle.units[battle.actor];
    const EnemyDef& definition = Enemy(self.species);
    const uint8_t victim = RandomLivingAlly(battle, rng);
    if (victim == kEmptySlot) {
        battle.phase = BattlePhase::kResolve;
        battle.wait_ms = 0U;
        return;
    }

    uint8_t chosen = kEmptySlot;
    uint8_t target = victim;
    if (definition.skill_count > 0U && rng.Below(100U) < 65U) {
        const uint8_t candidate = definition.skills[rng.Below(definition.skill_count)];
        const SkillDef& skill = Skill(candidate);
        if (skill.mp_cost <= self.mp) {
            switch (skill.kind) {
                case SkillKind::kHeal: {
                    // Only mend a foe that is actually hurt.
                    const uint8_t hurt = MostHurtOnSide(battle, true, 60U);
                    if (hurt != kEmptySlot) {
                        chosen = candidate;
                        target = skill.target == TargetSide::kOneAlly ? hurt : kEmptySlot;
                    }
                    break;
                }
                case SkillKind::kGuard:
                    if (!self.defending) {
                        chosen = candidate;
                        target = kEmptySlot;
                    }
                    break;
                default:
                    chosen = candidate;
                    break;
            }
        }
    }

    if (chosen != kEmptySlot) {
        PerformSkill(battle, rng, chosen, target);
    } else {
        PerformBasicAttack(battle, rng, victim);
    }
}

// --- phase machine ----------------------------------------------------------

void SelectActor(BattleState& battle, uint8_t unit_index, micropixel::XorShift32& rng) {
    battle.actor = unit_index;
    battle.cursor = 0U;
    battle.menu_rows = 0U;
    battle.target_dead = false;
    battle.units[unit_index].defending = false;  // a guard stance lasts one round
    if (battle.units[unit_index].enemy) {
        BeginEnemyAction(battle, rng);
    } else {
        battle.phase = BattlePhase::kCommand;
    }
}

void EndRound(BattleState& battle, micropixel::XorShift32& rng) {
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        Combatant& unit = battle.units[index];
        if (unit.alive && unit.poisoned) {
            const int32_t tick = micropixel::math::Clamp<int32_t>(static_cast<int32_t>(unit.max_hp) * 8 / 100, 3, 40);
            (void)DealDamage(battle, index, tick);
            battle.message = ids::Id::kBattlePoisonTick;
            battle.message_unit = index;
        }
        unit.defending = false;
        if (unit.def_mod_rounds > 0U) {
            --unit.def_mod_rounds;
            if (unit.def_mod_rounds == 0U) {
                unit.def_mod = 0;
            }
        }
    }
    (void)rng;
    ++battle.round;
    BuildOrder(battle);
    battle.order_index = 0U;
}

bool CheckOutcome(BattleState& battle, Progress& progress) {
    if (battle.phase == BattlePhase::kVictory || battle.phase == BattlePhase::kDefeat ||
        battle.phase == BattlePhase::kFled || battle.phase == BattlePhase::kFinished) {
        return true;
    }
    if (BattleLivingEnemies(battle) == 0U) {
        uint32_t xp = 0U;
        uint32_t gold = 0U;
        for (uint8_t index = 0U; index < battle.unit_count; ++index) {
            const Combatant& unit = battle.units[index];
            if (!unit.enemy) {
                continue;
            }
            const EnemyDef& definition = Enemy(unit.species);
            xp += definition.xp_reward;
            gold += definition.gold_reward;
            if (definition.drop_item != kItemNone && BagAdd(progress, definition.drop_item, 1U)) {
                battle.drop_item = definition.drop_item;
            }
        }
        battle.total_xp = xp;
        battle.total_gold = gold;
        progress.gold += gold;
        // Write the fight's HP back first: a level-up heals to full afterwards,
        // which the other order would have overwritten with stale battle values.
        SyncPartyFromBattle(battle, progress);
        for (uint8_t index = 0U; index < progress.party_size && index < kMaxParty; ++index) {
            if (progress.party[index].character != kEmptySlot && progress.party[index].hp == 0U) {
                progress.party[index].hp = 1U;  // no one is left down after the fight is won
            }
        }
        battle.level_ups = static_cast<uint8_t>(PartyGrantXp(progress, static_cast<uint16_t>(xp)));
        RestAfterBattle(progress, xp);
        battle.outcome = BattleOutcome::kWin;
        battle.phase = BattlePhase::kVictory;
        battle.message = ids::Id::kUiVictory;
        battle.message_unit = kEmptySlot;
        battle.wait_ms = 0U;
        return true;
    }
    if (BattleLivingParty(battle) == 0U) {
        SyncPartyFromBattle(battle, progress);
        // The party is carried back to the last rest point instead of hard-locking
        // the save; the chapter battle simply replays on the next attempt.
        PartyRestore(progress);
        battle.outcome = BattleOutcome::kLose;
        battle.phase = BattlePhase::kDefeat;
        battle.message = ids::Id::kUiDefeat;
        battle.message_unit = kEmptySlot;
        battle.wait_ms = 0U;
        return true;
    }
    return false;
}

void AdvanceActor(BattleState& battle, Progress& progress, micropixel::XorShift32& rng) {
    const uint8_t guard_limit = static_cast<uint8_t>(battle.unit_count * 2U + 4U);
    for (uint8_t guard = 0U; guard < guard_limit; ++guard) {
        if (CheckOutcome(battle, progress)) {
            return;
        }
        if (battle.order_index >= battle.order_count) {
            EndRound(battle, rng);
            if (CheckOutcome(battle, progress)) {
                return;
            }
            continue;
        }
        const uint8_t unit = battle.order[battle.order_index];
        if (battle.units[unit].alive) {
            SelectActor(battle, unit, rng);
            return;
        }
        ++battle.order_index;
    }
}

// --- menu helpers -----------------------------------------------------------

uint8_t CountBattleItems(const Progress& progress) {
    uint8_t count = 0U;
    for (uint8_t index = 0U; index < progress.bag_size && index < kMaxBag; ++index) {
        const BagSlot& slot = progress.bag[index];
        if (slot.item != kItemNone && slot.count > 0U && Item(slot.item).battle_usable) {
            ++count;
        }
    }
    return count;
}

uint8_t NthBattleItem(const Progress& progress, uint8_t row) {
    uint8_t seen = 0U;
    for (uint8_t index = 0U; index < progress.bag_size && index < kMaxBag; ++index) {
        const BagSlot& slot = progress.bag[index];
        if (slot.item == kItemNone || slot.count == 0U || !Item(slot.item).battle_usable) {
            continue;
        }
        if (seen == row) {
            return index;
        }
        ++seen;
    }
    return kEmptySlot;
}

void OpenSkillMenu(BattleState& battle) {
    battle.menu_rows =
        SkillUnlockedCount(battle.units[battle.actor].species, battle.units[battle.actor].level);
    battle.cursor = 0U;
    battle.return_phase = BattlePhase::kCommand;
    battle.phase = BattlePhase::kSkillMenu;
}

bool OpenItemMenu(BattleState& battle, const Progress& progress) {
    const uint8_t rows = CountBattleItems(progress);
    if (rows == 0U) {
        return false;
    }
    battle.menu_rows = rows;
    battle.cursor = 0U;
    battle.return_phase = BattlePhase::kCommand;
    battle.phase = BattlePhase::kItemMenu;
    return true;
}

void PerformPendingOnTarget(BattleState& battle, Progress& progress, micropixel::XorShift32& rng, uint8_t target) {
    switch (battle.pending_action) {
        case BattleAction::kAttack:
            PerformBasicAttack(battle, rng, target);
            break;
        case BattleAction::kSkill:
            PerformSkill(battle, rng, battle.pending_skill, target);
            break;
        case BattleAction::kItem:
            PerformItem(battle, progress, battle.pending_item, target);
            break;
        default:
            break;
    }
}

}  // namespace

// --- lifecycle --------------------------------------------------------------

void BattleBegin(BattleState& battle, const Progress& progress, uint8_t battle_id, micropixel::XorShift32& rng) {
    (void)rng;
    battle = BattleState{};
    battle.active = true;
    battle.battle_id = battle_id;
    battle.allow_flee = !Battle(battle_id).boss;

    uint8_t count = 0U;
    for (uint8_t index = 0U; index < progress.party_size && index < kMaxParty; ++index) {
        const PartyMember& member = progress.party[index];
        if (member.character == kEmptySlot) {
            continue;
        }
        Combatant& unit = battle.units[count];
        unit = Combatant{};
        unit.species = member.character;
        unit.slot = index;
        unit.level = member.level;
        unit.max_hp = MemberMaxHp(member);
        unit.max_mp = MemberMaxMp(member);
        unit.hp = member.hp;
        unit.mp = member.mp;
        unit.atk = MemberAttack(member);
        unit.def = MemberDefense(member);
        unit.spd = MemberSpeed(member);
        unit.poisoned = member.poisoned;
        unit.alive = member.hp > 0U;
        ++count;
    }

    const BattleDef& definition = Battle(battle_id);
    for (uint8_t index = 0U; index < definition.enemy_count && count < kMaxCombatants; ++index) {
        const EnemyDef& foe = Enemy(definition.enemies[index]);
        Combatant& unit = battle.units[count];
        unit = Combatant{};
        unit.enemy = true;
        unit.species = definition.enemies[index];
        unit.slot = index;
        unit.level = 1U;
        unit.max_hp = foe.hp;
        unit.hp = foe.hp;
        unit.max_mp = foe.mp;
        unit.mp = foe.mp;
        unit.atk = foe.atk;
        unit.def = foe.def;
        unit.spd = foe.spd;
        ++count;
    }

    battle.unit_count = count;
    battle.round = 1U;
    BuildOrder(battle);
    battle.order_index = 0U;
    battle.message = ids::Id::kBattleBegin;
    battle.message_unit = kEmptySlot;
    battle.phase = BattlePhase::kIntro;
    battle.wait_ms = kIntroMs;
}

void BattleUpdate(BattleState& battle, Progress& progress, uint32_t delta_ms, micropixel::XorShift32& rng) {
    if (!battle.active) {
        return;
    }
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        if (battle.popup_ms[index] == 0U) {
            continue;
        }
        battle.popup_ms[index] = battle.popup_ms[index] > delta_ms
                                     ? static_cast<uint16_t>(battle.popup_ms[index] - delta_ms)
                                     : 0U;
        if (battle.popup_ms[index] == 0U) {
            battle.popup[index] = 0;
        }
    }

    if (battle.phase != BattlePhase::kIntro && battle.phase != BattlePhase::kResolve) {
        return;
    }
    if (battle.wait_ms > delta_ms) {
        battle.wait_ms -= delta_ms;
        return;
    }
    battle.wait_ms = 0U;
    if (battle.phase == BattlePhase::kIntro) {
        // The opening splash hands over to the fastest unit of round one.
        battle.order_index = 0U;
    } else {
        ++battle.order_index;
    }
    AdvanceActor(battle, progress, rng);
}

// --- input ------------------------------------------------------------------

bool BattleMoveCursor(BattleState& battle, int32_t delta) {
    if (!battle.active) {
        return false;
    }
    const auto wrap = [](int32_t value, int32_t rows) {
        while (value < 0) {
            value += rows;
        }
        return static_cast<uint8_t>(rows > 0 ? value % rows : 0);
    };
    switch (battle.phase) {
        case BattlePhase::kCommand:
            battle.cursor = wrap(static_cast<int32_t>(battle.cursor) + delta, 5);
            return true;
        case BattlePhase::kSkillMenu:
        case BattlePhase::kItemMenu:
            if (battle.menu_rows == 0U) {
                return false;
            }
            battle.cursor = wrap(static_cast<int32_t>(battle.cursor) + delta, battle.menu_rows);
            return true;
        case BattlePhase::kTargetEnemy:
            battle.target = CycleTarget(battle, battle.target, delta, true, false);
            return true;
        case BattlePhase::kTargetAlly:
            battle.target = CycleTarget(battle, battle.target, delta, false, battle.target_dead);
            return true;
        default:
            return false;
    }
}

bool BattleConfirm(BattleState& battle, Progress& progress, micropixel::XorShift32& rng) {
    if (!battle.active) {
        return false;
    }
    switch (battle.phase) {
        case BattlePhase::kIntro:
        case BattlePhase::kResolve:
            battle.wait_ms = 0U;  // pressing skips the hold, not the action
            return true;
        case BattlePhase::kVictory:
        case BattlePhase::kDefeat:
        case BattlePhase::kFled:
            battle.phase = BattlePhase::kFinished;
            return true;
        case BattlePhase::kCommand:
            switch (battle.cursor) {
                case 0U:
                    battle.pending_action = BattleAction::kAttack;
                    break;
                case 1U:
                    OpenSkillMenu(battle);
                    return true;
                case 2U:
                    return OpenItemMenu(battle, progress);
                case 3U:
                    PerformDefend(battle);
                    return true;
                default:
                    TryFlee(battle, rng);
                    return true;
            }
            battle.target = FirstLivingEnemy(battle);
            if (battle.target == kEmptySlot) {
                return false;
            }
            battle.return_phase = BattlePhase::kCommand;
            battle.target_dead = false;
            battle.phase = BattlePhase::kTargetEnemy;
            return true;
        case BattlePhase::kSkillMenu: {
            if (battle.cursor >= battle.menu_rows) {
                return false;
            }
            const uint8_t skill_id = Character(battle.units[battle.actor].species).skills[battle.cursor];
            const SkillDef& skill = Skill(skill_id);
            if (battle.units[battle.actor].mp < skill.mp_cost) {
                return false;
            }
            battle.pending_action = BattleAction::kSkill;
            battle.pending_skill = skill_id;
            switch (skill.target) {
                case TargetSide::kOneEnemy:
                    battle.target = FirstLivingEnemy(battle);
                    if (battle.target == kEmptySlot) {
                        return false;
                    }
                    battle.return_phase = BattlePhase::kSkillMenu;
                    battle.target_dead = false;
                    battle.phase = BattlePhase::kTargetEnemy;
                    return true;
                case TargetSide::kOneAlly:
                    battle.target = FirstLivingAlly(battle);
                    if (battle.target == kEmptySlot) {
                        return false;
                    }
                    battle.return_phase = BattlePhase::kSkillMenu;
                    battle.target_dead = false;
                    battle.phase = BattlePhase::kTargetAlly;
                    return true;
                default:
                    PerformSkill(battle, rng, skill_id, kEmptySlot);
                    return true;
            }
        }
        case BattlePhase::kItemMenu: {
            if (battle.cursor >= battle.menu_rows) {
                return false;
            }
            const uint8_t bag_slot = NthBattleItem(progress, battle.cursor);
            if (bag_slot == kEmptySlot) {
                return false;
            }
            battle.pending_action = BattleAction::kItem;
            battle.pending_item = bag_slot;
            switch (Item(progress.bag[bag_slot].item).effect) {
                case ItemEffect::kHealHp:
                case ItemEffect::kHealMp:
                case ItemEffect::kCurePoison:
                    battle.target = FirstLivingAlly(battle);
                    if (battle.target == kEmptySlot) {
                        return false;
                    }
                    battle.return_phase = BattlePhase::kItemMenu;
                    battle.target_dead = false;
                    battle.phase = BattlePhase::kTargetAlly;
                    return true;
                case ItemEffect::kRevive:
                    battle.target = FirstFallenAlly(battle);
                    if (battle.target == kEmptySlot) {
                        return false;
                    }
                    battle.return_phase = BattlePhase::kItemMenu;
                    battle.target_dead = true;
                    battle.phase = BattlePhase::kTargetAlly;
                    return true;
                default:
                    PerformItem(battle, progress, bag_slot, kEmptySlot);
                    return true;
            }
        }
        case BattlePhase::kTargetEnemy:
        case BattlePhase::kTargetAlly:
            PerformPendingOnTarget(battle, progress, rng, battle.target);
            return true;
        default:
            return false;
    }
}

bool BattleBack(BattleState& battle) {
    if (!battle.active) {
        return false;
    }
    switch (battle.phase) {
        case BattlePhase::kSkillMenu:
        case BattlePhase::kItemMenu:
            battle.phase = BattlePhase::kCommand;
            battle.cursor = 0U;
            battle.menu_rows = 0U;
            return true;
        case BattlePhase::kTargetEnemy:
        case BattlePhase::kTargetAlly:
            battle.phase = battle.return_phase;
            battle.cursor = 0U;
            battle.target_dead = false;
            return true;
        default:
            return false;
    }
}

// --- queries ----------------------------------------------------------------

bool BattleFinished(const BattleState& battle) { return battle.phase == BattlePhase::kFinished; }
bool BattleWon(const BattleState& battle) { return battle.outcome == BattleOutcome::kWin; }
bool BattleLost(const BattleState& battle) { return battle.outcome == BattleOutcome::kLose; }
bool BattleFled(const BattleState& battle) { return battle.outcome == BattleOutcome::kFled; }

bool BattleCancellable(const BattleState& battle) {
    switch (battle.phase) {
        case BattlePhase::kSkillMenu:
        case BattlePhase::kItemMenu:
        case BattlePhase::kTargetEnemy:
        case BattlePhase::kTargetAlly:
            return true;
        default:
            return false;
    }
}

BattleMenu BattleCurrentMenu(const BattleState& battle) {
    switch (battle.phase) {
        case BattlePhase::kCommand:
            return BattleMenu::kRoot;
        case BattlePhase::kSkillMenu:
            return BattleMenu::kSkills;
        case BattlePhase::kItemMenu:
            return BattleMenu::kItems;
        default:
            return BattleMenu::kNone;
    }
}

uint8_t BattleMenuRows(const BattleState& battle, const Progress& progress) {
    (void)progress;
    switch (battle.phase) {
        case BattlePhase::kCommand:
            return 5U;
        case BattlePhase::kSkillMenu:
        case BattlePhase::kItemMenu:
            return battle.menu_rows;
        default:
            return 0U;
    }
}

uint8_t BattleMenuSkill(const BattleState& battle, uint8_t row) {
    const Combatant& actor = battle.units[battle.actor];
    return SkillRowSkill(actor.species, actor.level, row);
}

uint8_t BattleMenuItemSlot(const BattleState& battle, const Progress& progress, uint8_t row) {
    (void)battle;
    return NthBattleItem(progress, row);
}

bool BattleMenuRowEnabled(const BattleState& battle, const Progress& progress, uint8_t row) {
    switch (battle.phase) {
        case BattlePhase::kCommand:
            if (row == 4U) {
                return battle.allow_flee;
            }
            return true;
        case BattlePhase::kSkillMenu: {
            const uint8_t skill_id = BattleMenuSkill(battle, row);
            if (skill_id == kEmptySlot) {
                return false;
            }
            return battle.units[battle.actor].mp >= Skill(skill_id).mp_cost;
        }
        case BattlePhase::kItemMenu: {
            const uint8_t slot = NthBattleItem(progress, row);
            return slot != kEmptySlot;
        }
        default:
            return false;
    }
}

uint8_t BattleLivingEnemies(const BattleState& battle) {
    uint8_t count = 0U;
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        if (battle.units[index].enemy && battle.units[index].alive) {
            ++count;
        }
    }
    return count;
}

uint8_t BattleLivingParty(const BattleState& battle) {
    uint8_t count = 0U;
    for (uint8_t index = 0U; index < battle.unit_count; ++index) {
        if (!battle.units[index].enemy && battle.units[index].alive) {
            ++count;
        }
    }
    return count;
}

const Combatant& BattleUnit(const BattleState& battle, uint8_t index) {
    return battle.units[index < battle.unit_count ? index : 0U];
}

const Combatant& BattleActor(const BattleState& battle) { return battle.units[battle.actor]; }

}  // namespace pal
