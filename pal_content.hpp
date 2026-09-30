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
    kCharAnu = 3U,     // 阿奴 — the 苗疆 girl who replaces 林月如 after 锁妖塔
    kCharacterCount = 4U,
};

enum : uint8_t {
    kMapVillage = 0U,  // 余杭镇
    kMapForest = 1U,   // 十里坡
    kMapPass = 2U,     // 仙灵岛
    kMapHall = 3U,     // 水月宫
    kMapSuzhou = 4U,   // 苏州城
    kMapFort = 5U,     // 林家堡
    kMapShu = 6U,      // 蜀山
    kMapTower = 7U,    // 锁妖塔
    kMapAltar = 8U,    // 南诏祭坛
    kMapBaihe = 9U,    // 白河村
    kMapMiao = 10U,    // 苗疆
    kMapGrove = 11U,   // 神木林
    kMapTemple = 12U,  // 女娲神殿
    kMapSwordTomb = 13U,  // 蜀山剑冢
    kMapValley = 14U,     // 毒瘴谷
    kMapCount = 15U,
};

enum DialogueId : uint8_t {
    kDlgIntro = 0U,
    kDlgWolves,
    kDlgForest,
    kDlgBandits,
    kDlgPass,
    kDlgGhosts,
    kDlgHall,
    kDlgElder,
    kDlgKid,
    kDlgHostess,
    kDlgDepart,
    kDlgRoad,
    kDlgAfterHall,
    kDlgSuzhou,
    kDlgFort,
    kDlgFinal,
    kDlgShu,
    kDlgShuNpc,
    kDlgTower,
    kDlgTowerTop,
    kDlgAltar,
    kDlgEndGood,
    kDlgEndMid,
    kDlgEndBad,
    // Act 4-5: the fall of 锁妖塔 and the road to 女娲神殿.
    kDlgTowerFall,
    kDlgBaihe,
    kDlgFisher,
    kDlgBaiheWife,
    kDlgAnu,
    kDlgAnuChat,
    kDlgAnuAfter,
    kDlgGrove,
    kDlgQueenNpc,
    kDlgTemple,
    kDlgTempleAfter,
    kDlgStele,
    // Act 3 and 5 side chapters: 蜀山剑冢 and 毒瘴谷.
    kDlgSwordTomb,
    kDlgSwordAfter,
    kDlgSwordNpc,
    kDlgValley,
    kDlgValleyAfter,
    kDlgHerbalist,
    kDialogueCount,
};

enum BattleId : uint8_t {
    kBattleWolves = 0U,
    kBattleBandits,
    kBattleGhosts,
    kBattleBoss,
    kBattleChief,      // 山贼头目 ambush on 十里坡
    kBattleCultists,   // 拜月教徒 in 苏州城
    kBattleFort,       // the 林家堡 stand
    kBattleTrial,      // the 蜀山 trial
    kBattleTower,      // 锁妖塔 guard
    kBattlePriests,    // 拜月祭司
    kBattleOverlord,   // 拜月教主·真身 at 南诏祭坛
    kBattleBaihe,      // 毒蛛 outside 白河村
    kBattleMiao,       // 蛊师 in 苗疆
    kBattleGrove,      // 树妖 in 神木林
    kBattleGuardian,   // 水魔兽 under 女娲神殿
    kBattleSwordTomb,  // 剑灵 in 蜀山剑冢
    kBattleValley,     // 蛛后 in 毒瘴谷
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
inline constexpr uint32_t kFlagYunyangGone = 1U << 7U;   // 林月如 fell in 锁妖塔
inline constexpr uint32_t kFlagAnu = 1U << 8U;           // 阿奴 in the party
inline constexpr uint32_t kFlagTrust = 1U << 9U;         // won 阿奴's trust

// Bit index of kFlagYunyangGone, for StepKind::kFlag.
inline constexpr uint8_t kBitYunyangGone = 7U;

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
    kPoison,    // elemental damage that also poisons the target
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
// Elements (五灵) and skill unlocks
// ---------------------------------------------------------------------------

// Damage type of an attack. kElemNone means "no element": every physical
// strike, every utility skill, and anything that should ignore resistances.
enum : uint8_t {
    kElemNone = 0U,
    kElemFire = 1U,
    kElemWater = 2U,
    kElemThunder = 3U,
    kElemWind = 4U,
    kElemPoison = 5U,
    kElemCount = 6U,
};

// A skill slot that is not usable - locked by level, or past the list.
// Same value as pal_model's kEmptySlot; the content layer stays model-free.
inline constexpr uint8_t kNoSkill = 0xFFU;

uint8_t SkillElement(uint8_t skill_id);

// Signed percentage folded into incoming damage: +40 means the target takes 40%
// less of that element, -25 means 25% more. Anything not authored is neutral.
int16_t EnemyElementResist(uint8_t enemy_id, uint8_t element);
int16_t CharacterElementResist(uint8_t character, uint8_t element);

// Skills open up with levels instead of all arriving at level 1. Rows follow
// CharacterDef::skills order, so row 0 is always available.
uint8_t SkillUnlockedCount(uint8_t character, uint8_t level);
uint8_t SkillRowSkill(uint8_t character, uint8_t level, uint8_t row);

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
    kIdxLingzhi,
    kIdxOxhorn,
    kIdxJade,
    kIdxShuTalisman,
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
    bool boss;  // no escape from a story boss
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

inline constexpr uint8_t kMaxMapNpcs = 4U;
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
    kGrant,     // put `count` of item `arg` in the bag, then move on
    kFlag,      // set story flag bit `arg`, then move on
};

struct ScriptStepDef final {
    StepKind kind;
    uint8_t arg;
    uint8_t count{1U};
};

uint8_t ScriptCount();
const ScriptStepDef& ScriptStep(uint8_t index);

// Chooses the ending dialogue node from the accumulated story flags.
uint8_t EndingDialogueFor(uint32_t flags);

}  // namespace pal

#endif  // PAL_CONTENT_HPP
