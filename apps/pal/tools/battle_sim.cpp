// SPDX-License-Identifier: MIT
// Headless story simulator: plays the whole chapter script with a scripted
// "reasonable player" through the same Battle* entry points the scene uses, and
// reports how each fight goes. Built for wasm32-wasip1 and run under node by
// tools/run-battle-sim.ps1; it is not part of the shipped app.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../pal_battle.hpp"
#include "../pal_content.hpp"
#include "../pal_model.hpp"

using namespace pal;

namespace {

struct FightStats final {
    uint32_t attempts{};
    uint32_t wins{};
    uint32_t losses{};
    uint64_t rounds{};
    uint32_t stuck{};
};

FightStats g_stats[64];
uint32_t g_reach[128];  // runs that reached each script step
uint32_t g_party_level_sum[128];
uint32_t g_party_level_count[128];

enum class Policy : uint8_t { kSmart, kAttackOnly, kNoItems };

uint8_t WeakestAlly(const BattleState& battle, bool dead) {
    uint8_t best = kEmptySlot;
    uint32_t best_ratio = 0xFFFFFFFFU;
    for (uint8_t i = 0U; i < battle.unit_count; ++i) {
        const Combatant& unit = battle.units[i];
        if (unit.enemy || unit.alive == dead) {
            continue;
        }
        const uint32_t ratio = unit.max_hp == 0U ? 0U : unit.hp * 1000U / unit.max_hp;
        if (ratio < best_ratio) {
            best_ratio = ratio;
            best = i;
        }
    }
    return best;
}

uint8_t WeakestEnemy(const BattleState& battle) {
    uint8_t best = kEmptySlot;
    uint32_t best_hp = 0xFFFFFFFFU;
    for (uint8_t i = 0U; i < battle.unit_count; ++i) {
        const Combatant& unit = battle.units[i];
        if (!unit.enemy || !unit.alive) {
            continue;
        }
        if (unit.hp < best_hp) {
            best_hp = unit.hp;
            best = i;
        }
    }
    return best;
}

void Go(BattleState& battle, uint8_t from, uint8_t to) {
    (void)from;
    battle.cursor = to;
}

// Chooses and commits one hero action the way a touch on the UI would.
void PlayHeroTurn(BattleState& battle, Progress& progress, micropixel::XorShift32& rng, Policy policy) {
    const Combatant& actor = battle.units[battle.actor];
    const CharacterDef& character = Character(actor.species);
    const uint8_t weakest = WeakestAlly(battle, false);
    const uint8_t fallen = WeakestAlly(battle, true);
    const uint32_t weakest_ratio =
        weakest == kEmptySlot ? 1000U : battle.units[weakest].hp * 1000U / battle.units[weakest].max_hp;

    if (policy == Policy::kAttackOnly) {
        Go(battle, 0, 0);
        (void)BattleConfirm(battle, progress, rng);
        battle.target = WeakestEnemy(battle);
        (void)BattleConfirm(battle, progress, rng);
        return;
    }

    // Revive first, then emergency heal, then damage.
    if (policy == Policy::kSmart && fallen != kEmptySlot) {
        for (uint8_t slot = 0U; slot < progress.bag_size; ++slot) {
            if (progress.bag[slot].item != kItemNone && Item(progress.bag[slot].item).effect == ItemEffect::kRevive &&
                progress.bag[slot].count > 0U) {
                Go(battle, 0, 2);
                if (!BattleConfirm(battle, progress, rng)) {
                    break;
                }
                // Find the menu row of this bag slot.
                for (uint8_t row = 0U; row < battle.menu_rows; ++row) {
                    if (BattleMenuItemSlot(battle, progress, row) == slot) {
                        battle.cursor = row;
                    }
                }
                (void)BattleConfirm(battle, progress, rng);
                battle.target = fallen;
                (void)BattleConfirm(battle, progress, rng);
                return;
            }
        }
    }

    if (weakest_ratio < 420U) {
        // Healing skill?
        for (uint8_t s = 0U; s < character.skill_count; ++s) {
            const SkillDef& skill = Skill(character.skills[s]);
            if (skill.kind == SkillKind::kHeal && actor.mp >= skill.mp_cost) {
                Go(battle, 0, 1);
                (void)BattleConfirm(battle, progress, rng);
                battle.cursor = s;
                (void)BattleConfirm(battle, progress, rng);
                if (skill.target == TargetSide::kOneAlly) {
                    battle.target = weakest;
                    (void)BattleConfirm(battle, progress, rng);
                }
                return;
            }
        }
        if (policy == Policy::kSmart) {
            for (uint8_t slot = 0U; slot < progress.bag_size; ++slot) {
                const uint8_t item = progress.bag[slot].item;
                if (item != kItemNone && progress.bag[slot].count > 0U && Item(item).effect == ItemEffect::kHealHp &&
                    Item(item).battle_usable) {
                    Go(battle, 0, 2);
                    if (!BattleConfirm(battle, progress, rng)) {
                        break;
                    }
                    for (uint8_t row = 0U; row < battle.menu_rows; ++row) {
                        if (BattleMenuItemSlot(battle, progress, row) == slot) {
                            battle.cursor = row;
                        }
                    }
                    (void)BattleConfirm(battle, progress, rng);
                    battle.target = weakest;
                    (void)BattleConfirm(battle, progress, rng);
                    return;
                }
            }
        }
    }

    // Best damaging skill by expected total damage.
    uint32_t living = 0U;
    for (uint8_t i = 0U; i < battle.unit_count; ++i) {
        living += (battle.units[i].enemy && battle.units[i].alive) ? 1U : 0U;
    }
    int32_t best_score = 0;
    int32_t best_index = -1;
    for (uint8_t s = 0U; s < character.skill_count; ++s) {
        const SkillDef& skill = Skill(character.skills[s]);
        if (actor.mp < skill.mp_cost) {
            continue;
        }
        if (skill.kind != SkillKind::kPhysical && skill.kind != SkillKind::kMagic && skill.kind != SkillKind::kDrain) {
            continue;
        }
        int32_t score = skill.power;
        if (skill.target == TargetSide::kAllEnemies) {
            score = score * static_cast<int32_t>(living) * 8 / 10;
        }
        if (score > best_score) {
            best_score = score;
            best_index = s;
        }
    }
    if (best_index >= 0 && best_score > 130) {
        Go(battle, 0, 1);
        (void)BattleConfirm(battle, progress, rng);
        battle.cursor = static_cast<uint8_t>(best_index);
        (void)BattleConfirm(battle, progress, rng);
        if (battle.phase == BattlePhase::kTargetEnemy) {
            battle.target = WeakestEnemy(battle);
            (void)BattleConfirm(battle, progress, rng);
        }
        return;
    }
    Go(battle, 0, 0);
    (void)BattleConfirm(battle, progress, rng);
    battle.target = WeakestEnemy(battle);
    (void)BattleConfirm(battle, progress, rng);
}

// Returns true when the party won; losses restore the party and report false.
bool PlayBattle(Progress& progress, uint8_t battle_id, micropixel::XorShift32& rng, Policy policy) {
    BattleState battle{};
    BattleBegin(battle, progress, battle_id, rng);
    FightStats& stats = g_stats[battle_id];
    ++stats.attempts;
    for (uint32_t step = 0U; step < 20000U; ++step) {
        if (BattleFinished(battle)) {
            stats.rounds += battle.round;
            if (BattleWon(battle)) {
                ++stats.wins;
                return true;
            }
            ++stats.losses;
            return false;
        }
        switch (battle.phase) {
            case BattlePhase::kCommand:
                PlayHeroTurn(battle, progress, rng, policy);
                break;
            case BattlePhase::kVictory:
            case BattlePhase::kDefeat:
            case BattlePhase::kFled:
                (void)BattleConfirm(battle, progress, rng);
                break;
            default:
                BattleUpdate(battle, progress, 100U, rng);
                break;
        }
        if (battle.phase == BattlePhase::kTargetEnemy || battle.phase == BattlePhase::kTargetAlly ||
            battle.phase == BattlePhase::kSkillMenu || battle.phase == BattlePhase::kItemMenu) {
            // A menu the scripted player could not resolve: back out and attack.
            (void)BattleBack(battle);
            if (battle.phase == BattlePhase::kCommand) {
                battle.cursor = 0U;
                (void)BattleConfirm(battle, progress, rng);
                battle.target = WeakestEnemy(battle);
                (void)BattleConfirm(battle, progress, rng);
            }
        }
    }
    ++stats.stuck;
    return false;
}

void RunDialogue(Progress& progress, uint8_t node_id, micropixel::XorShift32& rng) {
    uint8_t node = node_id;
    for (uint8_t hops = 0U; hops < 8U; ++hops) {
        const DialogueNodeDef& def = DialogueNode(node);
        if (def.choice_count > 0U) {
            const ChoiceDef& choice = def.choices[rng.Below(def.choice_count)];
            progress.flags |= choice.set_flags;
            progress.flags &= ~choice.clear_flags;
            if (choice.next < 0) {
                return;
            }
            node = static_cast<uint8_t>(choice.next);
            continue;
        }
        if (def.next < 0) {
            return;
        }
        node = static_cast<uint8_t>(def.next);
    }
}

// One full playthrough; returns the highest script index reached.
uint32_t PlayThrough(micropixel::XorShift32& rng, Policy policy, uint32_t* retries_out) {
    Progress progress{};
    ProgressReset(progress);
    uint32_t retries = 0U;
    while (progress.script_index < ScriptCount()) {
        const uint8_t index = progress.script_index;
        ++g_reach[index];
        for (uint8_t m = 0U; m < progress.party_size; ++m) {
            g_party_level_sum[index] += progress.party[m].level;
            ++g_party_level_count[index];
        }
        const ScriptStepDef& step = ScriptStep(index);
        switch (step.kind) {
            case StepKind::kDialogue:
                RunDialogue(progress, step.arg, rng);
                PartySyncFromFlags(progress);
                break;
            case StepKind::kExplore:
                PartySyncFromFlags(progress);
                PartyRestore(progress);
                break;
            case StepKind::kBattle: {
                uint32_t tries = 0U;
                while (!PlayBattle(progress, step.arg, rng, policy)) {
                    PartyRestore(progress);
                    if (++tries >= 8U) {
                        *retries_out += tries;
                        return index;
                    }
                }
                retries += tries;
                break;
            }
            case StepKind::kGrant:
                (void)BagAdd(progress, step.arg, step.count);
                break;
            case StepKind::kFlag:
                progress.flags |= 1U << step.arg;
                break;
            default:
                break;
        }
        ++progress.script_index;
        PartySyncFromFlags(progress);
    }
    *retries_out += retries;
    return ScriptCount();
}

// --- static content validation ----------------------------------------------

uint32_t g_problems = 0U;

void Problem(const char* what, int32_t a, int32_t b) {
    printf("PROBLEM: %s (%d, %d)\n", what, a, b);
    ++g_problems;
}

// Flood-fills the walkable tiles of a map from its start tile.
void Flood(uint8_t map, bool reached[32][32]) {
    const MapDef& def = Map(map);
    int32_t queue[1024][2];
    uint32_t head = 0U;
    uint32_t tail = 0U;
    queue[tail][0] = MapStartX(map);
    queue[tail][1] = MapStartY(map);
    ++tail;
    reached[MapStartY(map)][MapStartX(map)] = true;
    while (head < tail) {
        const int32_t x = queue[head][0];
        const int32_t y = queue[head][1];
        ++head;
        const int32_t steps[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        for (const auto& step : steps) {
            const int32_t nx = x + step[0];
            const int32_t ny = y + step[1];
            if (nx < 0 || ny < 0 || nx >= def.width || ny >= def.height || reached[ny][nx]) {
                continue;
            }
            if (!MapTileWalkable(map, nx, ny)) {
                continue;
            }
            reached[ny][nx] = true;
            queue[tail][0] = nx;
            queue[tail][1] = ny;
            ++tail;
        }
    }
}

void ValidateContent() {
    for (uint8_t map = 0U; map < MapCount(); ++map) {
        const MapDef& def = Map(map);
        bool reached[32][32]{};
        Flood(map, reached);
        bool exit_reachable = false;
        for (uint8_t t = 0U; t < def.trigger_count; ++t) {
            const MapTriggerDef& trigger = def.triggers[t];
            for (uint8_t dy = 0U; dy < trigger.height; ++dy) {
                for (uint8_t dx = 0U; dx < trigger.width; ++dx) {
                    exit_reachable = exit_reachable || reached[trigger.y + dy][trigger.x + dx];
                }
            }
        }
        if (!exit_reachable) {
            Problem("map exit unreachable", map, 0);
        }
        if (!MapTileWalkable(map, MapStartX(map), MapStartY(map))) {
            Problem("map start not walkable", map, 0);
        }
        for (uint8_t n = 0U; n < def.npc_count; ++n) {
            const MapNpcDef& npc = def.npcs[n];
            const int32_t around[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            bool talkable = false;
            for (const auto& a : around) {
                const int32_t nx = npc.x + a[0];
                const int32_t ny = npc.y + a[1];
                talkable = talkable || (nx >= 0 && ny >= 0 && nx < def.width && ny < def.height && reached[ny][nx]);
            }
            if (!talkable) {
                Problem("npc cannot be reached", map, n);
            }
            if (npc.dialogue >= DialogueNodeCount()) {
                Problem("npc dialogue out of range", map, n);
            }
        }
    }
    for (uint8_t node = 0U; node < DialogueNodeCount(); ++node) {
        const DialogueNodeDef& def = DialogueNode(node);
        if (def.line_count == 0U || def.line_count > kMaxDialogueLines) {
            Problem("dialogue line count", node, def.line_count);
        }
        if (def.next >= static_cast<int32_t>(DialogueNodeCount())) {
            Problem("dialogue next out of range", node, def.next);
        }
        for (uint8_t c = 0U; c < def.choice_count; ++c) {
            if (def.choices[c].next >= static_cast<int32_t>(DialogueNodeCount())) {
                Problem("choice next out of range", node, c);
            }
        }
    }
    for (uint8_t id = 0U; id < BattleCount(); ++id) {
        const BattleDef& def = Battle(id);
        for (uint8_t e = 0U; e < def.enemy_count; ++e) {
            if (def.enemies[e] >= EnemyCount()) {
                Problem("battle enemy out of range", id, e);
            }
        }
    }
    for (uint8_t id = 0U; id < EnemyCount(); ++id) {
        const EnemyDef& def = Enemy(id);
        for (uint8_t s = 0U; s < def.skill_count; ++s) {
            if (def.skills[s] >= SkillCount()) {
                Problem("enemy skill out of range", id, s);
            }
        }
        if (def.drop_item != kItemNone && def.drop_item >= ItemCount()) {
            Problem("enemy drop out of range", id, def.drop_item);
        }
    }
    for (uint8_t id = 0U; id < CharacterCount(); ++id) {
        const CharacterDef& def = Character(id);
        for (uint8_t s = 0U; s < def.skill_count; ++s) {
            if (def.skills[s] >= SkillCount()) {
                Problem("character skill out of range", id, s);
            }
        }
    }
    for (uint8_t index = 0U; index < ScriptCount(); ++index) {
        const ScriptStepDef& step = ScriptStep(index);
        bool ok = true;
        switch (step.kind) {
            case StepKind::kDialogue:
                ok = step.arg < DialogueNodeCount();
                break;
            case StepKind::kExplore:
                ok = step.arg < MapCount();
                break;
            case StepKind::kBattle:
                ok = step.arg < BattleCount();
                break;
            case StepKind::kGrant:
                ok = step.arg < ItemCount() && step.count > 0U;
                break;
            case StepKind::kFlag:
                ok = step.arg < 32U;
                break;
            case StepKind::kEnding:
                ok = index + 1U == ScriptCount();
                break;
        }
        if (!ok) {
            Problem("script step invalid", index, static_cast<int32_t>(step.kind));
        }
    }
    for (uint8_t id = 0U; id < BattleCount(); ++id) {
        bool used = false;
        for (uint8_t index = 0U; index < ScriptCount(); ++index) {
            used = used || (ScriptStep(index).kind == StepKind::kBattle && ScriptStep(index).arg == id);
        }
        if (!used) {
            printf("note: battle %u is never used by the script\n", id);
        }
    }
    printf("content validation: %u problem(s), %u script steps, %u maps, %u dialogue nodes\n", g_problems,
           ScriptCount(), MapCount(), DialogueNodeCount());
}

void CheckSaveRoundTrip() {
    Progress progress{};
    ProgressReset(progress);
    progress.flags = kFlagLingxi | kFlagYunyang | kFlagAnu;
    progress.script_index = static_cast<uint8_t>(ScriptCount() - 1U);
    PartySyncFromFlags(progress);
    uint8_t blob[1024]{};
    const uint32_t size = ProgressSerialize(progress, blob, sizeof(blob));
    Progress loaded{};
    if (size == 0U || !ProgressDeserialize(loaded, blob, size) || loaded.script_index != progress.script_index ||
        loaded.party_size != progress.party_size || loaded.flags != progress.flags) {
        Problem("save round trip", size, 0);
    }
    // Older blobs must resume at the same story beat after the inserted chapters.
    const struct {
        uint8_t version;
        uint8_t saved;
        uint8_t expected;
    } cases[] = {{1U, 26U, 26U}, {1U, 27U, 28U}, {1U, 28U, 34U}, {1U, 33U, 63U}, {2U, 28U, 28U},
                 {2U, 29U, 34U}, {2U, 44U, 49U}, {2U, 45U, 55U}, {3U, 50U, 50U}};
    for (const auto& item : cases) {
        Progress old = progress;
        old.script_index = item.saved;
        const uint32_t old_size = ProgressSerialize(old, blob, sizeof(blob));
        blob[0] = item.version;
        Progress migrated{};
        if (old_size == 0U || !ProgressDeserialize(migrated, blob, old_size) ||
            migrated.script_index != item.expected) {
            Problem("save migration", item.version * 1000 + item.saved, migrated.script_index);
        }
    }
    printf("save checks done (%u bytes)\n", size);
}

const char* PolicyName(Policy policy) {
    switch (policy) {
        case Policy::kSmart:
            return "smart";
        case Policy::kAttackOnly:
            return "attack-only";
        default:
            return "skills-no-items";
    }
}

}  // namespace

int main(int argc, char** argv) {
    uint32_t runs = 300U;
    if (argc > 1) {
        runs = static_cast<uint32_t>(strtoul(argv[1], nullptr, 10));
    }
    ValidateContent();
    CheckSaveRoundTrip();
    if (g_problems > 0U) {
        return 1;
    }
    const Policy policies[] = {Policy::kSmart, Policy::kNoItems, Policy::kAttackOnly};
    for (const Policy policy : policies) {
        memset(g_stats, 0, sizeof(g_stats));
        memset(g_reach, 0, sizeof(g_reach));
        memset(g_party_level_sum, 0, sizeof(g_party_level_sum));
        memset(g_party_level_count, 0, sizeof(g_party_level_count));
        uint32_t completed = 0U;
        uint32_t retries = 0U;
        uint32_t furthest_hist[128]{};
        for (uint32_t run = 0U; run < runs; ++run) {
            micropixel::XorShift32 rng{0x1234U + run * 7919U};
            const uint32_t reached = PlayThrough(rng, policy, &retries);
            if (reached >= ScriptCount()) {
                ++completed;
            }
            ++furthest_hist[reached < 127U ? reached : 127U];
        }
        printf("== policy %s: %u/%u runs finished, %u battle retries total\n", PolicyName(policy), completed, runs,
               retries);
        for (uint8_t id = 0U; id < BattleCount(); ++id) {
            const FightStats& s = g_stats[id];
            if (s.attempts == 0U) {
                continue;
            }
            printf("  battle %2u: attempts %5u win %5.1f%% avg rounds %5.1f stuck %u\n", id, s.attempts,
                   100.0 * s.wins / s.attempts, s.attempts ? static_cast<double>(s.rounds) / s.attempts : 0.0,
                   s.stuck);
        }
        for (uint8_t index = 0U; index < ScriptCount(); ++index) {
            if (furthest_hist[index] > 0U) {
                printf("  gave up at script step %u: %u runs\n", index, furthest_hist[index]);
            }
        }
        printf("  avg party level at battles:");
        for (uint8_t index = 0U; index < ScriptCount(); ++index) {
            if (ScriptStep(index).kind == StepKind::kBattle && g_party_level_count[index] > 0U) {
                printf(" [%u]=%.1f", index,
                       static_cast<double>(g_party_level_sum[index]) / g_party_level_count[index]);
            }
        }
        printf("\n");
    }
    return 0;
}
