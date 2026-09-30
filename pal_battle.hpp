// SPDX-License-Identifier: Apache-2.0
// Turn-based battle engine.
//
// The engine owns a snapshot of both sides (party copies plus spawned enemies),
// a speed-sorted turn order and a small phase machine. The battle scene only
// reads BattleState to render and feeds input back through the Battle* calls,
// so combat rules stay free of any graphics dependency.

#ifndef PAL_BATTLE_HPP
#define PAL_BATTLE_HPP

#include <stdint.h>

#include "pal_content.hpp"
#include "pal_model.hpp"
#include "sdk/random.hpp"

namespace pal {

inline constexpr uint8_t kMaxCombatants = 6U;
inline constexpr uint16_t kPopupMs = 700U;

enum class BattlePhase : uint8_t {
    kIntro,       // opening splash, no input
    kCommand,     // root command menu for the active hero
    kSkillMenu,
    kItemMenu,
    kTargetEnemy,
    kTargetAlly,
    kResolve,     // message hold while an action plays out
    kVictory,
    kDefeat,
    kFled,
    kFinished,
};

enum class BattleAction : uint8_t { kAttack, kSkill, kItem, kDefend, kFlee };

enum class BattleMenu : uint8_t { kNone, kRoot, kSkills, kItems };

// Terminal result, kept after `phase` has already advanced to kFinished.
enum class BattleOutcome : uint8_t { kNone, kWin, kLose, kFled };

struct Combatant final {
    bool enemy{};
    uint8_t species{};  // character id for the party, enemy id for foes
    uint8_t slot{};     // index inside its own side
    uint16_t hp{}, max_hp{}, mp{}, max_mp{};
    uint8_t atk{}, def{}, spd{}, level{1U};
    bool alive{true};
    bool defending{};
    bool poisoned{};
    int8_t def_mod{};  // percentage applied to defence (冰缚咒)
    uint8_t def_mod_rounds{};
};

struct BattleState final {
    bool active{};
    bool allow_flee{true};
    uint8_t battle_id{};
    uint8_t round{1U};
    Combatant units[kMaxCombatants]{};
    uint8_t unit_count{};
    uint8_t order[kMaxCombatants]{};  // unit indices, fastest first
    uint8_t order_count{};
    uint8_t order_index{};
    BattlePhase phase{BattlePhase::kIntro};
    BattlePhase return_phase{BattlePhase::kCommand};
    BattleOutcome outcome{BattleOutcome::kNone};
    uint8_t actor{};          // unit index whose turn it is
    uint8_t cursor{};         // menu row
    uint8_t menu_rows{};      // rows in the open skill/item menu
    uint8_t target{};         // unit index being aimed at
    bool target_dead{};       // kTargetAlly is picking a fallen member
    uint8_t pending_skill{};
    uint8_t pending_item{};   // index into the bag
    BattleAction pending_action{BattleAction::kAttack};
    ids::Id message{};
    uint8_t message_unit{kEmptySlot};
    uint32_t wait_ms{};
    int16_t popup[kMaxCombatants]{};
    uint16_t popup_ms[kMaxCombatants]{};
    uint32_t total_xp{};
    uint32_t total_gold{};
    uint8_t level_ups{};
    uint8_t drop_item{kItemNone};
};

// --- lifecycle --------------------------------------------------------------

void BattleBegin(BattleState& battle, const Progress& progress, uint8_t battle_id, micropixel::XorShift32& rng);

// Drives timers, enemy turns and popups. Safe to call every tick.
void BattleUpdate(BattleState& battle, Progress& progress, uint32_t delta_ms, micropixel::XorShift32& rng);

// Writes the fight's HP/MP back into the party. Winning, losing and fleeing all
// settle the battle through this, so a retreat keeps the damage it took instead
// of healing the party for free.
void SyncPartyFromBattle(const BattleState& battle, Progress& progress);

// --- input ------------------------------------------------------------------
// Each returns true when the press was consumed by the battle UI.

bool BattleMoveCursor(BattleState& battle, int32_t delta);
bool BattleConfirm(BattleState& battle, Progress& progress, micropixel::XorShift32& rng);
bool BattleBack(BattleState& battle);

// --- queries ----------------------------------------------------------------

bool BattleFinished(const BattleState& battle);
bool BattleWon(const BattleState& battle);
bool BattleLost(const BattleState& battle);
bool BattleFled(const BattleState& battle);
bool BattleCancellable(const BattleState& battle);

BattleMenu BattleCurrentMenu(const BattleState& battle);
uint8_t BattleMenuRows(const BattleState& battle, const Progress& progress);
uint8_t BattleMenuSkill(const BattleState& battle, uint8_t row);
uint8_t BattleMenuItemSlot(const BattleState& battle, const Progress& progress, uint8_t row);
bool BattleMenuRowEnabled(const BattleState& battle, const Progress& progress, uint8_t row);

uint8_t BattleLivingEnemies(const BattleState& battle);
uint8_t BattleLivingParty(const BattleState& battle);
const Combatant& BattleUnit(const BattleState& battle, uint8_t index);
const Combatant& BattleActor(const BattleState& battle);

}  // namespace pal

#endif  // PAL_BATTLE_HPP
