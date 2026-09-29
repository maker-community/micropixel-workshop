// SPDX-License-Identifier: Apache-2.0
// The immutable content of 仙剑奇侠传: characters, skills, items, enemies, the
// dialogue graph, the exploration maps and the chapter script.
//
// This translation unit holds no logic beyond lookups and the walkability
// decoder, so designers can retune the game without touching the engine.

#include "pal_content.hpp"

namespace pal {
namespace {

// --- skill indices (referenced by characters and enemies) -------------------
enum : uint8_t {
    kSkSlash = 0U,
    kSkThrust,
    kSkWave,
    kSkThunder,
    kSkRain,
    kSkFrost,
    kSkCalm,
    kSkGuard,
    kSkRite,
    kSkDrain,
    kSkMiasma,
    kSkFlame,
    kSkCount,
};

constexpr SkillDef kSkills[kSkCount] = {
    {ids::Id::kSkillSlash, SkillKind::kPhysical, TargetSide::kOneEnemy, 0U, 130U, false},
    {ids::Id::kSkillThrust, SkillKind::kPhysical, TargetSide::kOneEnemy, 8U, 190U, false},
    {ids::Id::kSkillWave, SkillKind::kPhysical, TargetSide::kAllEnemies, 14U, 105U, false},
    {ids::Id::kSkillThunder, SkillKind::kMagic, TargetSide::kOneEnemy, 6U, 150U, false},
    {ids::Id::kSkillRain, SkillKind::kHeal, TargetSide::kAllAllies, 8U, 95U, false},
    {ids::Id::kSkillFrost, SkillKind::kBind, TargetSide::kOneEnemy, 10U, 0U, false},
    {ids::Id::kSkillCalm, SkillKind::kHeal, TargetSide::kOneAlly, 5U, 115U, true},
    {ids::Id::kSkillGuard, SkillKind::kGuard, TargetSide::kAllAllies, 8U, 40U, false},
    {ids::Id::kSkillRite, SkillKind::kMagic, TargetSide::kAllEnemies, 14U, 120U, false},
    {ids::Id::kSkillDrain, SkillKind::kDrain, TargetSide::kOneEnemy, 0U, 125U, false},
    {ids::Id::kSkillMiasma, SkillKind::kMagic, TargetSide::kAllEnemies, 0U, 100U, false},
    {ids::Id::kSkillFlame, SkillKind::kMagic, TargetSide::kOneEnemy, 0U, 185U, false},
};

// --- party ------------------------------------------------------------------
constexpr CharacterDef kCharacters[kCharacterCount] = {
    {ids::Id::kCharXiao, 0U, 120U, 30U, 26U, 16U, 15U, 18U, 4U, 4U, 3U, 2U, 3U, {kSkSlash, kSkThrust, kSkWave}},
    {ids::Id::kCharLingxi, 1U, 92U, 60U, 16U, 12U, 18U, 12U, 10U, 2U, 2U, 3U, 3U, {kSkThunder, kSkRain, kSkFrost}},
    {ids::Id::kCharYunyang, 2U, 108U, 55U, 18U, 15U, 13U, 14U, 9U, 3U, 3U, 2U, 3U, {kSkCalm, kSkGuard, kSkRite}},
};

// --- items ------------------------------------------------------------------
constexpr ItemDef kItems[kItemIdxCount] = {
    {ids::Id::kItemPillHp, ItemEffect::kHealHp, 80U, true, true},
    {ids::Id::kItemPillMp, ItemEffect::kHealMp, 40U, true, true},
    {ids::Id::kItemHerb, ItemEffect::kCurePoison, 0U, true, true},
    {ids::Id::kItemTalisman, ItemEffect::kDamageAll, 70U, true, false},
    {ids::Id::kItemRevive, ItemEffect::kRevive, 60U, true, true},
    {ids::Id::kItemRelic, ItemEffect::kKey, 0U, false, false},
    {ids::Id::kItemLingzhi, ItemEffect::kHealHp, 150U, true, true},
    {ids::Id::kItemOxhorn, ItemEffect::kDamageAll, 90U, true, false},
};

// --- enemies ----------------------------------------------------------------
enum : uint8_t {
    kFoeWolf = 0U,
    kFoeBandit,
    kFoeGhost,
    kFoeDemon,
    kFoeTreant,
    kFoeWasp,
    kFoeChief,
    kFoeCultist,
    kFoeCount,
};

constexpr EnemyDef kEnemies[kFoeCount] = {
    {ids::Id::kEnemyWolf, 0U, 90U, 0U, 18U, 6U, 12U, 0U, {0U, 0U}, 26U, 14U, kItemNone},
    {ids::Id::kEnemyBandit, 1U, 130U, 0U, 24U, 10U, 10U, 0U, {0U, 0U}, 38U, 22U, kIdxPillHp},
    {ids::Id::kEnemyGhost, 2U, 165U, 20U, 29U, 8U, 16U, 1U, {kSkDrain, 0U}, 54U, 30U, kIdxPillMp},
    {ids::Id::kEnemyDemon, 3U, 900U, 60U, 44U, 22U, 18U, 2U, {kSkMiasma, kSkFlame}, 420U, 320U, kIdxRevive},
    // 十里坡's slow wall: soaks hits and hits back hard.
    {ids::Id::kEnemyTreant, 3U, 260U, 0U, 24U, 18U, 6U, 1U, {kSkSlash, 0U}, 62U, 40U, kIdxLingzhi},
    // Fast, fragile, and the first foe that can poison the party.
    {ids::Id::kEnemyWasp, 0U, 110U, 0U, 20U, 6U, 22U, 1U, {kSkMiasma, 0U}, 34U, 18U, kIdxPillMp},
    // Act 2: the bandit captain fights like a player swordsman.
    {ids::Id::kEnemyChief, 1U, 320U, 30U, 30U, 14U, 14U, 2U, {kSkThrust, kSkSlash}, 92U, 64U, kIdxPillHp},
    // Act 2: cult casters, the first foes that answer with real magic.
    {ids::Id::kEnemyCultist, 3U, 240U, 40U, 26U, 12U, 18U, 2U, {kSkThunder, kSkFrost}, 110U, 72U, kIdxLingzhi},
};

constexpr BattleDef kBattles[kBattleCount] = {
    {2U, {kFoeWolf, kFoeWasp, 0U}},
    {3U, {kFoeBandit, kFoeBandit, kFoeTreant}},
    {2U, {kFoeGhost, kFoeGhost, 0U}},
    {3U, {kFoeDemon, kFoeGhost, kFoeTreant}},
    {2U, {kFoeChief, kFoeWasp, 0U}},
    {2U, {kFoeCultist, kFoeCultist, 0U}},
    {3U, {kFoeCultist, kFoeTreant, kFoeCultist}},
};

// --- dialogue graph ---------------------------------------------------------
constexpr DialogueNodeDef kDialogue[kDialogueCount] = {
    // 0 kDlgIntro — 酒剑仙 sends 李逍遥 to 仙灵岛
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryIntroL0, 0xFFU, 0U},
      {ids::Id::kCharMaster, ids::Id::kStoryIntroL1, 3U, 0U},
      {ids::Id::kCharXiao, ids::Id::kStoryIntroL2, 0U, 1U}},
     0U,
     {},
     -1},
    // 1 kDlgWolves — 赵灵儿 joins
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryWolvesL0, 0xFFU, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryWolvesL1, 1U, 3U},
      {ids::Id::kCharLingxi, ids::Id::kStoryWolvesL2, 1U, 0U}},
     2U,
     {{ids::Id::kStoryWolvesC0, -1, kFlagRelic | kFlagLingxi, 0U},
      {ids::Id::kStoryWolvesC1, -1, kFlagLingxi, 0U},
      {}},
     -1},
    // 2 kDlgForest — the villagers ambushed on 十里坡
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryForestL0, 0xFFU, 0U},
      {ids::Id::kCharMerchant, ids::Id::kStoryForestL1, 4U, 2U},
      {ids::Id::kCharLingxi, ids::Id::kStoryForestL2, 1U, 0U}},
     3U,
     {{ids::Id::kStoryForestC0, -1, kFlagMercy, kFlagBold | kFlagCalm},
      {ids::Id::kStoryForestC1, -1, kFlagBold, kFlagMercy | kFlagCalm},
      {ids::Id::kStoryForestC2, -1, kFlagCalm, kFlagMercy | kFlagBold}},
     -1},
    // 3 kDlgBandits — aftermath of the ambush
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryBanditsL0, 0xFFU, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryBanditsL1, 1U, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryBanditsL2, 1U, 2U}},
     0U,
     {},
     -1},
    // 4 kDlgPass — 林月如 joins
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryPassL0, 0xFFU, 0U},
      {ids::Id::kCharYunyang, ids::Id::kStoryPassL1, 2U, 0U},
      {ids::Id::kCharYunyang, ids::Id::kStoryPassL2, 2U, 3U}},
     2U,
     {{ids::Id::kStoryPassC0, -1, kFlagYunyang | kFlagCourtesy, 0U},
      {ids::Id::kStoryPassC1, -1, kFlagYunyang, kFlagCourtesy},
      {}},
     -1},
    // 5 kDlgGhosts — the blood writing on the pass
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryGhostsL0, 0xFFU, 0U},
      {ids::Id::kCharYunyang, ids::Id::kStoryGhostsL1, 2U, 1U},
      {ids::Id::kCharYunyang, ids::Id::kStoryGhostsL2, 2U, 0U}},
     0U,
     {},
     -1},
    // 6 kDlgHall — facing 拜月教主
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryHallL0, 0xFFU, 0U},
      {ids::Id::kCharDemon, ids::Id::kStoryHallL1, 5U, 1U},
      {ids::Id::kCharLingxi, ids::Id::kStoryHallL2, 1U, 0U}},
     0U,
     {},
     -1},
    // 7 kDlgElder — 吴伯 points the way south
    {2U,
     {{ids::Id::kCharElder, ids::Id::kStoryElderL0, 4U, 0U},
      {ids::Id::kCharElder, ids::Id::kStoryElderL1, 4U, 2U},
      {}},
     0U,
     {},
     -1},
    // 8 kDlgKid — the children's rumour about the island
    {2U,
     {{ids::Id::kCharKid, ids::Id::kStoryKidL0, 4U, 3U},
      {ids::Id::kCharKid, ids::Id::kStoryKidL1, 4U, 0U},
      {}},
     0U,
     {},
     -1},
    // 9 kDlgHostess — 婶婶 sees 李逍遥 off
    {2U,
     {{ids::Id::kCharHostess, ids::Id::kStoryHostessL0, 4U, 2U},
      {ids::Id::kCharHostess, ids::Id::kStoryHostessL1, 4U, 0U},
      {}},
     0U,
     {},
     -1},
    // 10 kDlgDepart — 酒剑仙's briefing before the road opens
    {3U,
     {{ids::Id::kCharMaster, ids::Id::kStoryDepartL0, 3U, 0U},
      {ids::Id::kCharMaster, ids::Id::kStoryDepartL1, 3U, 0U},
      {ids::Id::kCharXiao, ids::Id::kStoryDepartL2, 0U, 3U}},
     0U,
     {},
     -1},
    // 11 kDlgRoad — the fork at 十里坡口; the pick colours the ending
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryRoadL0, 0U, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryRoadL1, 1U, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryRoadL2, 1U, 0U}},
     2U,
     {{ids::Id::kStoryRoadC0, -1, kFlagBold, 0U},
      {ids::Id::kStoryRoadC1, -1, kFlagCalm, 0U},
      {}},
     -1},
    // 12 kDlgAfterHall — 拜月教主 slips away and points the party at 苏州
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryAfterhallL0, 0U, 0U},
      {ids::Id::kCharYunyang, ids::Id::kStoryAfterhallL1, 2U, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryAfterhallL2, 1U, 2U}},
     0U,
     {},
     -1},
    // 13 kDlgSuzhou — arrival in 苏州城
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStorySuzhouL0, 0U, 0U},
      {ids::Id::kCharXiao, ids::Id::kStorySuzhouL1, 0U, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStorySuzhouL2, 1U, 3U}},
     0U,
     {},
     -1},
    // 14 kDlgFort — the 林家堡 stand
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryFortL0, 0U, 0U},
      {ids::Id::kCharYunyang, ids::Id::kStoryFortL1, 2U, 1U},
      {ids::Id::kCharLingxi, ids::Id::kStoryFortL2, 1U, 0U}},
     0U,
     {},
     -1},
    // 15 kDlgFinal — the chapter closes with the cult still standing
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryFinalL0, 0U, 3U},
      {ids::Id::kCharLingxi, ids::Id::kStoryFinalL1, 1U, 0U},
      {ids::Id::kCharXiao, ids::Id::kStoryFinalL2, 0U, 0U}},
     0U,
     {},
     -1},
    // 16 kDlgEndGood
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryEndGoodL0, 0xFFU, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryEndGoodL1, 1U, 3U},
      {ids::Id::kCharXiao, ids::Id::kStoryEndGoodL2, 0U, 3U}},
     0U,
     {},
     -1},
    // 17 kDlgEndMid
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryEndMidL0, 0xFFU, 0U},
      {ids::Id::kCharYunyang, ids::Id::kStoryEndMidL1, 2U, 0U},
      {ids::Id::kCharXiao, ids::Id::kStoryEndMidL2, 0U, 2U}},
     0U,
     {},
     -1},
    // 18 kDlgEndBad
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryEndBadL0, 0xFFU, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryEndBadL1, 1U, 2U},
      {ids::Id::kCharXiao, ids::Id::kStoryEndBadL2, 0U, 2U}},
     0U,
     {},
     -1},
};

// --- maps -------------------------------------------------------------------
const char* const kVillageRows[] = {
    "TTTTTTTTTTTTTTTT",
    "T..............T",
    "T.RRRR....RRRR.T",
    "T.RRRR....RRRR.T",
    "T..............T",
    "T......::......T",
    "T......::......T",
    "T.RRRR.:..RRRR.T",
    "T.RRRR.:..RRRR.T",
    "T......::......T",
    "T......**......T",
    "TTTTTTTTTTTTTTTT",
};

const char* const kForestRows[] = {
    "TTTTTTTTTTTTTTTT",
    "T..............T",
    "T.TTTT....TTTT.T",
    "T.TTTT....TTTT.T",
    "T..............T",
    "T......::......T",
    "T......::......T",
    "T.TTTT.:..TTTT.T",
    "T.TTTT.:..TTTT.T",
    "T......::......T",
    "T......**......T",
    "TTTTTTTTTTTTTTTT",
};

const char* const kPassRows[] = {
    "################",
    "#______________#",
    "#______________#",
    "#___##____##___#",
    "#___##____##___#",
    "#______________#",
    "#______________#",
    "#___##____##___#",
    "#___##____##___#",
    "#______________#",
    "#______**______#",
    "################",
};

const char* const kHallRows[] = {
    "############",
    "#__________#",
    "#_##____##_#",
    "#_##____##_#",
    "#__________#",
    "#__________#",
    "#_##____##_#",
    "#_##____##_#",
    "#____**____#",
    "############",
};

// 苏州城 — canals cut the town into islands of houses.
const char* const kSuzhouRows[] = {
    "TTTTTTTTTTTTTTTT",
    "T....~~~~......T",
    "T....~~~~......T",
    "T....~~~~..RRR.T",
    "T..........RRR.T",
    "T..RRRR........T",
    "T..RRRR...~~~~.T",
    "T.........~~~~.T",
    "T..RRRR...~~~~.T",
    "T..RRRR........T",
    "T......**......T",
    "TTTTTTTTTTTTTTTT",
};

// 林家堡 — an open courtyard with colonnades, entered from the north.
const char* const kFortRows[] = {
    "################",
    "#______________#",
    "#___##____##___#",
    "#___##____##___#",
    "#______________#",
    "#______________#",
    "#___##____##___#",
    "#___##____##___#",
    "#______________#",
    "#______________#",
    "#______**______#",
    "################",
};

constexpr MapDef kMaps[kMapCount] = {
    {ids::Id::kMapVillage,
     16U,
     12U,
     kVillageRows,
     4U,
     {{ids::Id::kCharMaster, 3U, 7U, 3U, kDlgIntro},
      {ids::Id::kCharElder, 4U, 2U, 1U, kDlgElder},
      {ids::Id::kCharKid, 4U, 13U, 1U, kDlgKid},
      {ids::Id::kCharHostess, 4U, 3U, 4U, kDlgHostess}},
     1U,
     {{7U, 10U, 2U, 1U}, {}}},
    {ids::Id::kMapForest,
     16U,
     12U,
     kForestRows,
     1U,
     {{ids::Id::kCharMerchant, 4U, 5U, 4U, kDlgBandits}, {}},
     1U,
     {{7U, 10U, 2U, 1U}, {}}},
    {ids::Id::kMapPass,
     16U,
     12U,
     kPassRows,
     1U,
     {{ids::Id::kCharYunyang, 2U, 5U, 5U, kDlgPass}, {}},
     1U,
     {{7U, 10U, 2U, 1U}, {}}},
    {ids::Id::kMapHall,
     12U,
     10U,
     kHallRows,
     0U,
     {{}, {}, {}, {}},
     1U,
     {{5U, 8U, 2U, 1U}, {}}},
    {ids::Id::kMapSuzhou,
     16U,
     12U,
     kSuzhouRows,
     0U,
     {{}, {}, {}, {}},
     1U,
     {{7U, 10U, 2U, 1U}, {}}},
    {ids::Id::kMapFort,
     16U,
     12U,
     kFortRows,
     0U,
     {{}, {}, {}, {}},
     1U,
     {{7U, 10U, 2U, 1U}, {}}},
};

// Starting tile per map (the tile the party stands on when a chapter opens).
constexpr uint8_t kMapStartX[kMapCount] = {7U, 7U, 7U, 5U, 7U, 7U};
constexpr uint8_t kMapStartY[kMapCount] = {4U, 1U, 1U, 1U, 4U, 1U};

// --- chapter script ---------------------------------------------------------
constexpr ScriptStepDef kScript[] = {
    {StepKind::kDialogue, kDlgIntro},
    {StepKind::kExplore, kMapVillage},
    {StepKind::kDialogue, kDlgDepart},
    {StepKind::kBattle, kBattleWolves},
    {StepKind::kDialogue, kDlgWolves},
    {StepKind::kExplore, kMapForest},
    {StepKind::kDialogue, kDlgForest},
    {StepKind::kBattle, kBattleChief},
    {StepKind::kDialogue, kDlgBandits},
    {StepKind::kDialogue, kDlgRoad},
    {StepKind::kExplore, kMapPass},
    {StepKind::kDialogue, kDlgPass},
    {StepKind::kBattle, kBattleGhosts},
    {StepKind::kDialogue, kDlgGhosts},
    {StepKind::kExplore, kMapHall},
    {StepKind::kDialogue, kDlgHall},
    {StepKind::kBattle, kBattleBoss},
    {StepKind::kDialogue, kDlgAfterHall},
    {StepKind::kExplore, kMapSuzhou},
    {StepKind::kDialogue, kDlgSuzhou},
    {StepKind::kBattle, kBattleCultists},
    {StepKind::kExplore, kMapFort},
    {StepKind::kDialogue, kDlgFort},
    {StepKind::kBattle, kBattleFort},
    {StepKind::kDialogue, kDlgFinal},
    {StepKind::kEnding, 0U},
};

Tile DecodeTile(char code) {
    switch (code) {
        case ':':
            return Tile::kPath;
        case 'T':
            return Tile::kTree;
        case '~':
            return Tile::kWater;
        case 'R':
            return Tile::kRoof;
        case '#':
            return Tile::kWall;
        case '_':
            return Tile::kFloor;
        case '*':
            return Tile::kShrine;
        default:
            return Tile::kGrass;
    }
}

}  // namespace

uint8_t SkillCount() { return kSkCount; }
const SkillDef& Skill(uint8_t id) { return kSkills[id < kSkCount ? id : 0U]; }

uint8_t CharacterCount() { return kCharacterCount; }
const CharacterDef& Character(uint8_t id) { return kCharacters[id < kCharacterCount ? id : 0U]; }

uint8_t ItemCount() { return kItemIdxCount; }
const ItemDef& Item(uint8_t id) { return kItems[id < kItemIdxCount ? id : 0U]; }

uint8_t EnemyCount() { return kFoeCount; }
const EnemyDef& Enemy(uint8_t id) { return kEnemies[id < kFoeCount ? id : 0U]; }

uint8_t BattleCount() { return kBattleCount; }
const BattleDef& Battle(uint8_t id) { return kBattles[id < kBattleCount ? id : 0U]; }

uint8_t DialogueNodeCount() { return kDialogueCount; }
const DialogueNodeDef& DialogueNode(uint8_t id) { return kDialogue[id < kDialogueCount ? id : 0U]; }

uint8_t MapCount() { return kMapCount; }
const MapDef& Map(uint8_t id) { return kMaps[id < kMapCount ? id : 0U]; }

Tile MapTileAt(uint8_t map_id, int32_t x, int32_t y) {
    const MapDef& map = Map(map_id);
    if (x < 0 || y < 0 || x >= static_cast<int32_t>(map.width) || y >= static_cast<int32_t>(map.height)) {
        return Tile::kWall;
    }
    return DecodeTile(map.rows[y][x]);
}

bool MapTileWalkable(uint8_t map_id, int32_t x, int32_t y) {
    const MapDef& map = Map(map_id);
    if (x < 0 || y < 0 || x >= static_cast<int32_t>(map.width) || y >= static_cast<int32_t>(map.height)) {
        return false;
    }
    // Bodies block the tile they stand on so the player cannot walk through an NPC.
    for (uint8_t index = 0U; index < map.npc_count; ++index) {
        if (map.npcs[index].x == x && map.npcs[index].y == y) {
            return false;
        }
    }
    switch (DecodeTile(map.rows[y][x])) {
        case Tile::kTree:
        case Tile::kWater:
        case Tile::kRoof:
        case Tile::kWall:
            return false;
        default:
            return true;
    }
}

uint8_t ScriptCount() { return static_cast<uint8_t>(sizeof(kScript) / sizeof(kScript[0])); }
const ScriptStepDef& ScriptStep(uint8_t index) {
    return kScript[index < ScriptCount() ? index : 0U];
}

uint8_t MapStartX(uint8_t map_id) { return kMapStartX[map_id < kMapCount ? map_id : 0U]; }
uint8_t MapStartY(uint8_t map_id) { return kMapStartY[map_id < kMapCount ? map_id : 0U]; }

uint8_t EndingDialogueFor(uint32_t flags) {
    const bool relic = (flags & kFlagRelic) != 0U;
    const bool mercy = (flags & kFlagMercy) != 0U;
    const bool bold = (flags & kFlagBold) != 0U;
    const bool courtesy = (flags & kFlagCourtesy) != 0U;
    if (relic && mercy) {
        return kDlgEndGood;
    }
    if (bold || courtesy) {
        return kDlgEndMid;
    }
    return kDlgEndBad;
}

}  // namespace pal
