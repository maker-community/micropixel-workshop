// SPDX-License-Identifier: Apache-2.0
// Static content tables for 仙剑奇侠传.
//
// Everything here is immutable data: characters, skills, items, enemies, the
// dialogue graph, the exploration maps and the chapter script. All user-facing
// text is referenced by generated localization id, never inlined, so the game
// ships a single translation catalog (i18n/zh-CN.json).

#ifndef PAL_CONTENT_HPP
#define PAL_CONTENT_HPP

#include <stdint.h>

#include "pal_strings.hpp"

namespace pal {
namespace ids = pal_strings;

// ---------------------------------------------------------------------------
// Identifiers
// ---------------------------------------------------------------------------

enum : uint8_t {
    kCharXiao = 0U,    // 李逍遥 — the innkeeper's nephew
    kCharLingxi = 1U,  // 赵灵儿 — the water-spirit maiden
    kCharYunyang = 2U, // 林月如 — the swordswoman of 林家堡
    kCharacterCount = 3U,
};

enum : uint8_t {
    kMapVillage = 0U,  // 余杭镇
    kMapForest = 1U,   // 十里坡
    kMapPass = 2U,     // 仙灵岛
    kMapHall = 3U,     // 水月宫
    kMapCount = 4U,
};

enum DialogueId : uint8_t {
    kDlgIntro = 0U,
    kDlgWolves,
    kDlgForest,
    kDlgBandits,
    kDlgPass,
    kDlgGhosts,
    kDlgHall,
    kDlgEndGood,
    kDlgEndMid,
    kDlgEndBad,
    kDialogueCount,
};

enum BattleId : uint8_t {
    kBattleWolves = 0U,
    kBattleBandits,
    kBattleGhosts,
    kBattleBoss,
    kBattleCount,
};

// Story flags persisted in the save blob.
inline constexpr uint32_t kFlagRelic = 1U << 0U;         // accepted 水灵珠
inline constexpr uint32_t kFlagLingxi = 1U << 1U;        // 赵灵儿 in the party
inline constexpr uint32_t kFlagMercy = 1U << 2U;         // saved the villagers
inline constexpr uint32_t kFlagBold = 1U << 3U;          // flanked the bandits
inline constexpr uint32_t kFlagCalm = 1U << 4U;          // waited and watched
inline constexpr uint32_t kFlagYunyang = 1U << 5U;       // 林月如 in the party
inline constexpr uint32_t kFlagCourtesy = 1U << 6U;      // invited her politely

inline constexpr uint8_t kItemNone = 0xFFU;

// ---------------------------------------------------------------------------
// Skills
// ---------------------------------------------------------------------------

enum class SkillKind : uint8_t {
    kPhysical,  // power scales a physical strike
    kMagic,     // power scales an elemental strike
    kHeal,      // restores HP
    kGuard,     // raises defence of the target side
    kBind,      // lowers defence of one enemy
    kDrain,     // damage plus caster self-heal
};

enum class TargetSide : uint8_t {
    kOneEnemy,
    kAllEnemies,
    kOneAlly,
    kAllAllies,
    kSelf,
};

struct SkillDef final {
    ids::Id name;
    SkillKind kind;
    TargetSide target;
    uint8_t mp_cost;
    uint8_t power;
    bool field_usable;  // castable outside battle (healing only)
};

uint8_t SkillCount();
const SkillDef& Skill(uint8_t id);

// ---------------------------------------------------------------------------
// Playable characters
// ---------------------------------------------------------------------------

inline constexpr uint8_t kMaxCharacterSkills = 4U;

struct CharacterDef final {
    ids::Id name;
    uint8_t portrait;  // art selector, see widgets::DrawPortrait
    uint8_t base_hp;
    uint8_t base_mp;
    uint8_t base_atk;
    uint8_t base_def;
    uint8_t base_spd;
    uint8_t growth_hp;
    uint8_t growth_mp;
    uint8_t growth_atk;
    uint8_t growth_def;
    uint8_t growth_spd;
    uint8_t skill_count;
    uint8_t skills[kMaxCharacterSkills];
};

uint8_t CharacterCount();
const CharacterDef& Character(uint8_t id);

// ---------------------------------------------------------------------------
// Items
// ---------------------------------------------------------------------------

enum class ItemEffect : uint8_t {
    kHealHp,
    kHealMp,
    kCurePoison,
    kDamageAll,
    kRevive,
    kKey,  // story item, not consumed from the bag
};

struct ItemDef final {
    ids::Id name;
    ItemEffect effect;
    uint8_t magnitude;
    bool battle_usable;
    bool field_usable;
};

// Row order of the item table; referenced by the save file and drop tables.
enum : uint8_t {
    kIdxPillHp = 0U,
    kIdxPillMp,
    kIdxHerb,
    kIdxTalisman,
    kIdxRevive,
    kIdxRelic,
    kItemIdxCount,
};

uint8_t ItemCount();
const ItemDef& Item(uint8_t id);

// ---------------------------------------------------------------------------
// Enemies
// ---------------------------------------------------------------------------

inline constexpr uint8_t kMaxEnemySkills = 2U;

struct EnemyDef final {
    ids::Id name;
    uint8_t sprite;  // art selector, see widgets::DrawEnemy
    uint16_t hp;
    uint16_t mp;
    uint8_t atk;
    uint8_t def;
    uint8_t spd;
    uint8_t skill_count;
    uint8_t skills[kMaxEnemySkills];
    uint16_t xp_reward;
    uint16_t gold_reward;
    uint8_t drop_item;  // kItemNone when the enemy drops nothing
};

uint8_t EnemyCount();
const EnemyDef& Enemy(uint8_t id);

// ---------------------------------------------------------------------------
// Battles
// ---------------------------------------------------------------------------

inline constexpr uint8_t kMaxBattleEnemies = 3U;

struct BattleDef final {
    uint8_t enemy_count;
    uint8_t enemies[kMaxBattleEnemies];
};

uint8_t BattleCount();
const BattleDef& Battle(uint8_t id);

// ---------------------------------------------------------------------------
// Dialogue graph
// ---------------------------------------------------------------------------

inline constexpr uint8_t kMaxDialogueLines = 3U;
inline constexpr uint8_t kMaxChoices = 3U;

struct DialogueLineDef final {
    ids::Id speaker;  // kCharX? / kCharMaster / kCharMerchant / kCharDemon
    ids::Id text;
    uint8_t portrait;  // art selector for the speaker
    uint8_t emotion;   // 0 calm, 1 angry, 2 sad, 3 glad
};

struct ChoiceDef final {
    ids::Id text;
    int8_t next;          // dialogue node, or -1 to return to the chapter script
    uint32_t set_flags;   // OR-ed into the save flags when picked
    uint32_t clear_flags; // AND-ed out of the save flags when picked
};

struct DialogueNodeDef final {
    uint8_t line_count;
    DialogueLineDef lines[kMaxDialogueLines];
    uint8_t choice_count;  // 0 -> advance to `next`
    ChoiceDef choices[kMaxChoices];
    int8_t next;  // -1 -> return to the chapter script
};

uint8_t DialogueNodeCount();
const DialogueNodeDef& DialogueNode(uint8_t id);

// ---------------------------------------------------------------------------
// Exploration maps
// ---------------------------------------------------------------------------

enum class Tile : uint8_t {
    kGrass,
    kPath,
    kTree,
    kWater,
    kRoof,
    kWall,
    kFloor,
    kShrine,
};

inline constexpr uint8_t kMaxMapNpcs = 2U;
inline constexpr uint8_t kMaxMapTriggers = 2U;

struct MapNpcDef final {
    ids::Id name;
    uint8_t portrait;
    uint8_t x;
    uint8_t y;
    uint8_t dialogue;  // dialogue node started when the player faces the NPC
};

struct MapTriggerDef final {
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
};

struct MapDef final {
    ids::Id name;
    uint8_t width;
    uint8_t height;
    // `height` NUL-terminated rows of `width` ASCII tile codes, row major.
    // '.' grass  ':' path  'T' tree  '~' water  'R' roof
    // '#' wall   '_' floor '*' shrine (the chapter exit)
    const char* const* rows;
    uint8_t npc_count;
    MapNpcDef npcs[kMaxMapNpcs];
    uint8_t trigger_count;
    MapTriggerDef triggers[kMaxMapTriggers];
};

uint8_t MapCount();
const MapDef& Map(uint8_t id);
Tile MapTileAt(uint8_t map_id, int32_t x, int32_t y);
bool MapTileWalkable(uint8_t map_id, int32_t x, int32_t y);

// Tile the party stands on when `map_id` opens.
uint8_t MapStartX(uint8_t map_id);
uint8_t MapStartY(uint8_t map_id);

// ---------------------------------------------------------------------------
// Chapter script
// ---------------------------------------------------------------------------

enum class StepKind : uint8_t {
    kDialogue,  // play dialogue node `arg`
    kExplore,   // walk map `arg` until its trigger fires
    kBattle,    // win battle `arg`
    kEnding,    // roll the credits based on the collected flags
};

struct ScriptStepDef final {
    StepKind kind;
    uint8_t arg;
};

uint8_t ScriptCount();
const ScriptStepDef& ScriptStep(uint8_t index);

// Chooses the ending dialogue node from the accumulated story flags.
uint8_t EndingDialogueFor(uint32_t flags);

}  // namespace pal

#endif  // PAL_CONTENT_HPP
