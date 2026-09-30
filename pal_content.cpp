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
    kSkSwordRain,
    kSkWaterDragon,
    kSkRedLotus,
    kSkGuDu,
    kSkSerpent,
    kSkSpring,
    kSkSwarm,
    kSkTide,
    kSkCount,
};

constexpr SkillDef kSkills[] = {
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
    {ids::Id::kSkillMiasma, SkillKind::kPoison, TargetSide::kAllEnemies, 0U, 60U, false},
    {ids::Id::kSkillFlame, SkillKind::kMagic, TargetSide::kOneEnemy, 0U, 185U, false},
    // Act 3 ultimates, one per hero, so the late fights are winnable.
    {ids::Id::kSkillSwordRain, SkillKind::kPhysical, TargetSide::kAllEnemies, 20U, 175U, false},
    {ids::Id::kSkillWaterDragon, SkillKind::kMagic, TargetSide::kAllEnemies, 18U, 150U, false},
    {ids::Id::kSkillRedLotus, SkillKind::kMagic, TargetSide::kOneEnemy, 14U, 230U, false},
    // Act 4-5: 阿奴's poisons and swarms, and the water beast's tide.
    {ids::Id::kSkillGudu, SkillKind::kPoison, TargetSide::kOneEnemy, 6U, 110U, false},
    {ids::Id::kSkillSerpent, SkillKind::kMagic, TargetSide::kAllEnemies, 12U, 110U, false},
    {ids::Id::kSkillSpring, SkillKind::kHeal, TargetSide::kAllAllies, 14U, 130U, false},
    {ids::Id::kSkillSwarm, SkillKind::kMagic, TargetSide::kAllEnemies, 20U, 170U, false},
    {ids::Id::kSkillTide, SkillKind::kMagic, TargetSide::kAllEnemies, 16U, 120U, false},
};

// --- party ------------------------------------------------------------------
constexpr CharacterDef kCharacters[] = {
    {ids::Id::kCharXiao, 0U, 120U, 30U, 26U, 16U, 15U, 18U, 4U, 4U, 3U, 2U, 4U,
     {kSkSlash, kSkThrust, kSkWave, kSkSwordRain}},
    {ids::Id::kCharLingxi, 1U, 92U, 60U, 16U, 12U, 18U, 12U, 10U, 2U, 2U, 3U, 4U,
     {kSkThunder, kSkRain, kSkFrost, kSkWaterDragon}},
    {ids::Id::kCharYunyang, 2U, 108U, 55U, 18U, 15U, 13U, 14U, 9U, 3U, 3U, 2U, 4U,
     {kSkCalm, kSkGuard, kSkRite, kSkRedLotus}},
    {ids::Id::kCharAnu, 6U, 100U, 52U, 20U, 13U, 17U, 13U, 9U, 3U, 2U, 3U, 4U,
     {kSkGuDu, kSkSerpent, kSkSpring, kSkSwarm}},
};

// --- items ------------------------------------------------------------------
constexpr ItemDef kItems[] = {
    {ids::Id::kItemPillHp, ItemEffect::kHealHp, 80U, true, true},
    {ids::Id::kItemPillMp, ItemEffect::kHealMp, 40U, true, true},
    {ids::Id::kItemHerb, ItemEffect::kCurePoison, 0U, true, true},
    {ids::Id::kItemTalisman, ItemEffect::kDamageAll, 70U, true, false},
    {ids::Id::kItemRevive, ItemEffect::kRevive, 60U, true, true},
    {ids::Id::kItemRelic, ItemEffect::kKey, 0U, false, false},
    {ids::Id::kItemLingzhi, ItemEffect::kHealHp, 150U, true, true},
    {ids::Id::kItemOxhorn, ItemEffect::kDamageAll, 90U, true, false},
    {ids::Id::kItemJade, ItemEffect::kHealHp, 250U, true, true},
    {ids::Id::kItemShuTalisman, ItemEffect::kDamageAll, 150U, true, false},
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
    kFoeSerpent,
    kFoeGolem,
    kFoePriest,
    kFoeOverlord,
    kFoeSpider,
    kFoeShaman,
    kFoeTreeSpirit,
    kFoeWaterBeast,
    kFoeSwordSpirit,
    kFoeSpiderQueen,
    kFoeCount,
};

constexpr EnemyDef kEnemies[] = {
    {ids::Id::kEnemyWolf, 0U, 70U, 0U, 16U, 6U, 12U, 0U, {0U, 0U}, 26U, 14U, kItemNone},
    {ids::Id::kEnemyBandit, 1U, 130U, 0U, 24U, 10U, 10U, 0U, {0U, 0U}, 38U, 22U, kIdxPillHp},
    {ids::Id::kEnemyGhost, 2U, 165U, 20U, 29U, 8U, 16U, 1U, {kSkDrain, 0U}, 54U, 30U, kIdxPillMp},
    {ids::Id::kEnemyDemon, 3U, 900U, 60U, 44U, 22U, 18U, 2U, {kSkMiasma, kSkFlame}, 420U, 320U, kIdxRevive},
    // 十里坡's slow wall: soaks hits and hits back hard.
    {ids::Id::kEnemyTreant, 6U, 260U, 0U, 24U, 18U, 6U, 1U, {kSkSlash, 0U}, 62U, 40U, kIdxLingzhi},
    // Fast, fragile, and the first foe that can poison the party.
    {ids::Id::kEnemyWasp, 0U, 95U, 0U, 20U, 6U, 22U, 1U, {kSkMiasma, 0U}, 34U, 18U, kIdxPillMp},
    // Act 2: the bandit captain fights like a player swordsman.
    {ids::Id::kEnemyChief, 1U, 210U, 30U, 26U, 12U, 14U, 2U, {kSkThrust, kSkSlash}, 92U, 64U, kIdxPillHp},
    // Act 2: cult casters, the first foes that answer with real magic.
    {ids::Id::kEnemyCultist, 3U, 380U, 40U, 30U, 18U, 18U, 2U, {kSkThunder, kSkFrost}, 110U, 72U, kIdxLingzhi},
    // Act 3. The tower's serpents are fast, its golems are a wall, and the
    // priests heal the thing the party is trying to kill.
    {ids::Id::kEnemySerpent, 0U, 420U, 20U, 36U, 18U, 20U, 2U, {kSkMiasma, kSkFrost}, 96U, 60U, kIdxHerb},
    {ids::Id::kEnemyGolem, 3U, 800U, 24U, 42U, 30U, 5U, 2U, {kSkSlash, kSkGuard}, 130U, 80U, kIdxLingzhi},
    {ids::Id::kEnemyPriest, 3U, 520U, 120U, 38U, 20U, 16U, 2U, {kSkRite, kSkDrain}, 160U, 100U, kIdxPillMp},
    {ids::Id::kEnemyOverlord, 3U, 3600U, 400U, 118U, 44U, 24U, 2U, {kSkFlame, kSkRite}, 800U, 600U, kIdxJade},
    // Act 4-5: 白河/苗疆's venom, the grove's living wood and the temple's guardian.
    {ids::Id::kEnemySpider, 4U, 640U, 0U, 46U, 24U, 18U, 1U, {kSkMiasma, 0U}, 130U, 70U, kIdxHerb},
    {ids::Id::kEnemyShaman, 3U, 700U, 60U, 48U, 24U, 15U, 2U, {kSkGuDu, kSkFrost}, 170U, 100U, kIdxLingzhi},
    {ids::Id::kEnemyTreespirit, 6U, 1000U, 60U, 56U, 32U, 8U, 2U, {kSkCalm, kSkSlash}, 210U, 120U, kIdxLingzhi},
    {ids::Id::kEnemyWaterbeast, 5U, 3400U, 320U, 108U, 42U, 19U, 2U, {kSkTide, kSkSlash}, 1000U, 700U, kIdxJade},
    // Side chapters: 蜀山剑冢's blades and 毒瘴谷's queen.
    {ids::Id::kEnemySwordspirit, 7U, 600U, 40U, 44U, 26U, 21U, 2U, {kSkThrust, kSkSlash}, 140U, 90U, kIdxLingzhi},
    {ids::Id::kEnemySpiderqueen, 4U, 2600U, 120U, 80U, 34U, 20U, 2U, {kSkMiasma, kSkGuDu}, 320U, 200U, kIdxJade},
};

constexpr BattleDef kBattles[] = {
    {2U, {kFoeWolf, kFoeWolf, 0U}, false},
    {3U, {kFoeBandit, kFoeBandit, kFoeTreant}, false},
    {2U, {kFoeGhost, kFoeGhost, 0U}, false},
    {3U, {kFoeDemon, kFoeGhost, kFoeTreant}, true},
    {2U, {kFoeChief, kFoeWasp, 0U}, false},
    {2U, {kFoeCultist, kFoeCultist, 0U}, false},
    {3U, {kFoeCultist, kFoeSerpent, kFoeCultist}, false},
    {2U, {kFoeGolem, kFoeSerpent, 0U}, false},
    {3U, {kFoeSerpent, kFoeGolem, kFoeSerpent}, false},
    {3U, {kFoePriest, kFoeCultist, kFoePriest}, true},
    {3U, {kFoeOverlord, kFoePriest, kFoeGolem}, true},
    {3U, {kFoeSpider, kFoeSpider, kFoeWasp}, false},
    {3U, {kFoeShaman, kFoeSpider, kFoeShaman}, false},
    {3U, {kFoeTreeSpirit, kFoeTreant, kFoeTreeSpirit}, false},
    {3U, {kFoeWaterBeast, kFoeSerpent, kFoeSerpent}, true},
    {3U, {kFoeSwordSpirit, kFoeSwordSpirit, kFoeGolem}, false},
    {3U, {kFoeSpiderQueen, kFoeSpider, kFoeSpider}, true},
};

// --- dialogue graph ---------------------------------------------------------
constexpr DialogueNodeDef kDialogue[] = {
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
    // 16 kDlgShu — 蜀山掌门 explains what the cult is really after
    {3U,
     {{ids::Id::kCharShu, ids::Id::kStoryShuL0, 8U, 0U},
      {ids::Id::kCharShu, ids::Id::kStoryShuL1, 8U, 0U},
      {ids::Id::kCharShu, ids::Id::kStoryShuL2, 8U, 0U}},
     0U,
     {},
     -1},
    // 17 kDlgShuNpc — the sect leader's aside on 蜀山
    {2U,
     {{ids::Id::kCharShu, ids::Id::kStoryShunpcL0, 8U, 2U},
      {ids::Id::kCharShu, ids::Id::kStoryShunpcL1, 8U, 0U},
      {}},
     0U,
     {},
     -1},
    // 18 kDlgTower — entering 锁妖塔
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryTowerL0, 0U, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryTowerL1, 1U, 2U},
      {ids::Id::kCharYunyang, ids::Id::kStoryTowerL2, 2U, 0U}},
     0U,
     {},
     -1},
    // 19 kDlgTowerTop — 赵灵儿 learns where she comes from
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryTowertopL0, 0U, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryTowertopL1, 1U, 2U},
      {ids::Id::kCharXiao, ids::Id::kStoryTowertopL2, 0U, 3U}},
     0U,
     {},
     -1},
    // 20 kDlgAltar — the 南诏 showdown
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryAltarL0, 0U, 0U},
      {ids::Id::kCharDemon, ids::Id::kStoryAltarL1, 5U, 2U},
      {ids::Id::kCharXiao, ids::Id::kStoryAltarL2, 0U, 1U}},
     0U,
     {},
     -1},
    // 21 kDlgEndGood
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryEndGoodL0, 0xFFU, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryEndGoodL1, 1U, 3U},
      {ids::Id::kCharXiao, ids::Id::kStoryEndGoodL2, 0U, 3U}},
     0U,
     {},
     -1},
    // 22 kDlgEndMid
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryEndMidL0, 0xFFU, 0U},
      {ids::Id::kCharAnu, ids::Id::kStoryEndMidL1, 6U, 0U},
      {ids::Id::kCharXiao, ids::Id::kStoryEndMidL2, 0U, 2U}},
     0U,
     {},
     -1},
    // 23 kDlgEndBad
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryEndBadL0, 0xFFU, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryEndBadL1, 1U, 2U},
      {ids::Id::kCharXiao, ids::Id::kStoryEndBadL2, 0U, 2U}},
     0U,
     {},
     -1},
    // 24 kDlgTowerFall — 林月如 holds the seal shut and does not come back
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryTowerfallL0, 0xFFU, 1U},
      {ids::Id::kCharYunyang, ids::Id::kStoryTowerfallL1, 2U, 2U},
      {ids::Id::kCharXiao, ids::Id::kStoryTowerfallL2, 0U, 2U}},
     0U,
     {},
     -1},
    // 25 kDlgBaihe — 白河村 after the fall; 老渔夫 hands over antidotes
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryBaiheL0, 0xFFU, 2U},
      {ids::Id::kCharLingxi, ids::Id::kStoryBaiheL1, 1U, 2U},
      {ids::Id::kCharFisher, ids::Id::kStoryBaiheL2, 4U, 0U}},
     0U,
     {},
     -1},
    // 26 kDlgFisher — 白河村 rumour
    {2U,
     {{ids::Id::kCharFisher, ids::Id::kStoryFisherL0, 4U, 0U},
      {ids::Id::kCharFisher, ids::Id::kStoryFisherL1, 4U, 2U},
      {}},
     0U,
     {},
     -1},
    // 27 kDlgBaiheWife — 白河村 grief
    {2U,
     {{ids::Id::kCharMerchant, ids::Id::kStoryBaihewifeL0, 4U, 2U},
      {ids::Id::kCharMerchant, ids::Id::kStoryBaihewifeL1, 4U, 0U},
      {}},
     0U,
     {},
     -1},
    // 28 kDlgAnu — 阿奴 joins in 苗疆
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryAnuL0, 0xFFU, 0U},
      {ids::Id::kCharAnu, ids::Id::kStoryAnuL1, 6U, 3U},
      {ids::Id::kCharAnu, ids::Id::kStoryAnuL2, 6U, 3U}},
     2U,
     {{ids::Id::kStoryAnuC0, -1, kFlagAnu | kFlagTrust, 0U},
      {ids::Id::kStoryAnuC1, -1, kFlagAnu, kFlagTrust},
      {}},
     -1},
    // 29 kDlgAnuChat — 阿奴 on the bridge
    {2U,
     {{ids::Id::kCharAnu, ids::Id::kStoryAnuchatL0, 6U, 3U},
      {ids::Id::kCharAnu, ids::Id::kStoryAnuchatL1, 6U, 0U},
      {}},
     0U,
     {},
     -1},
    // 30 kDlgAnuAfter — 阿奴 hands over her father's horns
    {3U,
     {{ids::Id::kCharAnu, ids::Id::kStoryAnuafterL0, 6U, 3U},
      {ids::Id::kCharAnu, ids::Id::kStoryAnuafterL1, 6U, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryAnuafterL2, 1U, 0U}},
     0U,
     {},
     -1},
    // 31 kDlgGrove — 巫后 tells 灵儿 who she is
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryGroveL0, 0xFFU, 0U},
      {ids::Id::kCharQueen, ids::Id::kStoryGroveL1, 7U, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryGroveL2, 1U, 2U}},
     0U,
     {},
     -1},
    // 32 kDlgQueenNpc — 巫后's aside
    {2U,
     {{ids::Id::kCharQueen, ids::Id::kStoryQueennpcL0, 7U, 0U},
      {ids::Id::kCharQueen, ids::Id::kStoryQueennpcL1, 7U, 2U},
      {}},
     0U,
     {},
     -1},
    // 33 kDlgTemple — 女娲神殿, the guardian rises
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryTempleL0, 0xFFU, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryTempleL1, 1U, 0U},
      {ids::Id::kCharXiao, ids::Id::kStoryTempleL2, 0U, 1U}},
     0U,
     {},
     -1},
    // 34 kDlgTempleAfter — 灵儿 awakens; a bottle of 天仙玉露 is left behind
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryTempleafterL0, 0xFFU, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryTempleafterL1, 1U, 3U},
      {ids::Id::kCharAnu, ids::Id::kStoryTempleafterL2, 6U, 3U}},
     0U,
     {},
     -1},
    // 35 kDlgStele — the temple's inscription
    {2U,
     {{ids::Id::kCharStele, ids::Id::kStorySteleL0, 0xFFU, 0U},
      {ids::Id::kCharStele, ids::Id::kStorySteleL1, 0xFFU, 0U},
      {}},
     0U,
     {},
     -1},
    // 36 kDlgSwordTomb — 酒剑仙 leads the party into 蜀山剑冢
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStorySwordtombL0, 0xFFU, 0U},
      {ids::Id::kCharMaster, ids::Id::kStorySwordtombL1, 3U, 3U},
      {ids::Id::kCharXiao, ids::Id::kStorySwordtombL2, 0U, 1U}},
     0U,
     {},
     -1},
    // 37 kDlgSwordAfter — 万剑归宗, and two 千年灵芝
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStorySwordafterL0, 0xFFU, 0U},
      {ids::Id::kCharMaster, ids::Id::kStorySwordafterL1, 3U, 3U},
      {ids::Id::kCharMaster, ids::Id::kStorySwordafterL2, 3U, 0U}},
     0U,
     {},
     -1},
    // 38 kDlgSwordNpc — a disciple's aside
    {2U,
     {{ids::Id::kCharDisciple, ids::Id::kStorySwordnpcL0, 4U, 0U},
      {ids::Id::kCharDisciple, ids::Id::kStorySwordnpcL1, 4U, 3U},
      {}},
     0U,
     {},
     -1},
    // 39 kDlgValley — into the purple mist
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryValleyL0, 0xFFU, 0U},
      {ids::Id::kCharAnu, ids::Id::kStoryValleyL1, 6U, 0U},
      {ids::Id::kCharLingxi, ids::Id::kStoryValleyL2, 1U, 2U}},
     0U,
     {},
     -1},
    // 40 kDlgValleyAfter — 蛛后 was guarding the grove; three antidotes
    {3U,
     {{ids::Id::kCharXiao, ids::Id::kStoryValleyafterL0, 0xFFU, 0U},
      {ids::Id::kCharAnu, ids::Id::kStoryValleyafterL1, 6U, 2U},
      {ids::Id::kCharAnu, ids::Id::kStoryValleyafterL2, 6U, 3U}},
     0U,
     {},
     -1},
    // 41 kDlgHerbalist — 毒瘴谷 rumour
    {2U,
     {{ids::Id::kCharHerbalist, ids::Id::kStoryHerbalistL0, 4U, 0U},
      {ids::Id::kCharHerbalist, ids::Id::kStoryHerbalistL1, 4U, 3U},
      {}},
     0U,
     {},
     -1},
};

// --- maps -------------------------------------------------------------------
constexpr const char* kVillageRows[] = {
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

constexpr const char* kForestRows[] = {
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

constexpr const char* kPassRows[] = {
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

constexpr const char* kHallRows[] = {
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
constexpr const char* kSuzhouRows[] = {
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
constexpr const char* kFortRows[] = {
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

// 蜀山 — stone platforms and spirit pools along the ridge.
constexpr const char* kShuRows[] = {
    "TTTTTTTTTTTTTTTT",
    "T..~~~~........T",
    "T..~~~~..####..T",
    "T........####..T",
    "T..RRRR........T",
    "T..RRRR...RRRR.T",
    "T.........RRRR.T",
    "T..~~~~........T",
    "T..~~~~..####..T",
    "T........####..T",
    "T......**......T",
    "TTTTTTTTTTTTTTTT",
};

// 锁妖塔 — a warren of cells broken by load-bearing walls.
constexpr const char* kTowerRows[] = {
    "################",
    "#___#____#_____#",
    "#___#____#_____#",
    "#___######_____#",
    "#______________#",
    "#_####_####_####",
    "#______________#",
    "#_###_####_#####",
    "#______________#",
    "#_####_####____#",
    "#______**______#",
    "################",
};

// 南诏祭坛 — an arena ringed by water, with the moon over the array.
constexpr const char* kAltarRows[] = {
    "~~~~~~~~~~~~~~~~",
    "~______________~",
    "~___##____##___~",
    "~___##____##___~",
    "~______________~",
    "~______________~",
    "~___##____##___~",
    "~___##____##___~",
    "~______________~",
    "~______________~",
    "~_____****_____~",
    "~~~~~~~~~~~~~~~~",
};

// 白河村 — a river village at the foot of 锁妖塔's shadow.
constexpr const char* kBaiheRows[] = {
    "TTTTTTTTTTTTTTTT",
    "T~~~~..........T",
    "T~~~~...RRRR...T",
    "T.......RRRR...T",
    "T..RRR.........T",
    "T..RRR.::......T",
    "T......::..~~~.T",
    "T.RRR..::......T",
    "T.RRR..::.~~...T",
    "T......::......T",
    "T......**......T",
    "TTTTTTTTTTTTTTTT",
};

// 苗疆 — stilt houses over slow water, bamboo groves at the edges.
constexpr const char* kMiaoRows[] = {
    "TTTTTTTTTTTTTTTT",
    "T..TT......TT..T",
    "T..TT..~~..TT..T",
    "T.......~~.....T",
    "T..RRR.........T",
    "T..RRR.::......T",
    "T......::..~~..T",
    "T.RRR..::......T",
    "T.RRR..::..TT..T",
    "T......::......T",
    "T......**......T",
    "TTTTTTTTTTTTTTTT",
};

// 神木林 — the sacred grove; the trees close in around a single path.
constexpr const char* kGroveRows[] = {
    "TTTTTTTTTTTTTTTT",
    "T..TTT....TTT..T",
    "T..TTT....TTT..T",
    "T..............T",
    "T.TT.......TT..T",
    "T.TT...::......T",
    "T......::..TTT.T",
    "T.TTT..::......T",
    "T.TTT..::...~~.T",
    "T......::......T",
    "T......**......T",
    "TTTTTTTTTTTTTTTT",
};

// 女娲神殿 — a flooded hall with two reflecting pools.
constexpr const char* kTempleRows[] = {
    "################",
    "#______________#",
    "#__~~~____~~~__#",
    "#__~~~____~~~__#",
    "#______________#",
    "#_##________##_#",
    "#_##________##_#",
    "#______________#",
    "#___~~____~~___#",
    "#______________#",
    "#______**______#",
    "################",
};

// 蜀山剑冢 — a stone hall of buried blades around a spirit pool.
constexpr const char* kSwordTombRows[] = {
    "################",
    "#______________#",
    "#_##__~~~~__##_#",
    "#_##__~~~~__##_#",
    "#______________#",
    "#__###____###__#",
    "#______________#",
    "#__###____###__#",
    "#______________#",
    "#______________#",
    "#______**______#",
    "################",
};

// 毒瘴谷 — a mist-choked ravine; the path is the only clean ground.
constexpr const char* kValleyRows[] = {
    "TTTTTTTTTTTTTTTT",
    "T~~..TT....~~..T",
    "T~~..TT....~~..T",
    "T..............T",
    "T..TTT.....TT..T",
    "T..TTT.::......T",
    "T......::...~~.T",
    "T.TT...::......T",
    "T.TT...::..TT..T",
    "T......::......T",
    "T......**......T",
    "TTTTTTTTTTTTTTTT",
};

constexpr MapDef kMaps[] = {
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
    {ids::Id::kMapShu,
     16U,
     12U,
     kShuRows,
     1U,
     {{ids::Id::kCharShu, 8U, 9U, 4U, kDlgShuNpc}, {}, {}, {}},
     1U,
     {{7U, 10U, 2U, 1U}, {}}},
    {ids::Id::kMapTower,
     16U,
     12U,
     kTowerRows,
     0U,
     {{}, {}, {}, {}},
     1U,
     {{7U, 10U, 2U, 1U}, {}}},
    {ids::Id::kMapAltar,
     16U,
     12U,
     kAltarRows,
     0U,
     {{}, {}, {}, {}},
     1U,
     {{6U, 10U, 4U, 1U}, {}}},
    {ids::Id::kMapBaihe,
     16U,
     12U,
     kBaiheRows,
     2U,
     {{ids::Id::kCharFisher, 4U, 5U, 2U, kDlgFisher}, {ids::Id::kCharMerchant, 4U, 10U, 5U, kDlgBaiheWife}, {}, {}},
     1U,
     {{7U, 10U, 2U, 1U}, {}}},
    {ids::Id::kMapMiao,
     16U,
     12U,
     kMiaoRows,
     1U,
     {{ids::Id::kCharAnu, 6U, 9U, 5U, kDlgAnuChat}, {}, {}, {}},
     1U,
     {{7U, 10U, 2U, 1U}, {}}},
    {ids::Id::kMapGrove,
     16U,
     12U,
     kGroveRows,
     1U,
     {{ids::Id::kCharQueen, 7U, 11U, 3U, kDlgQueenNpc}, {}, {}, {}},
     1U,
     {{7U, 10U, 2U, 1U}, {}}},
    {ids::Id::kMapTemple,
     16U,
     12U,
     kTempleRows,
     1U,
     {{ids::Id::kCharStele, 3U, 7U, 2U, kDlgStele}, {}, {}, {}},
     1U,
     {{7U, 10U, 2U, 1U}, {}}},
    {ids::Id::kMapSwordtomb,
     16U,
     12U,
     kSwordTombRows,
     1U,
     {{ids::Id::kCharDisciple, 4U, 3U, 4U, kDlgSwordNpc}, {}, {}, {}},
     1U,
     {{7U, 10U, 2U, 1U}, {}}},
    {ids::Id::kMapValley,
     16U,
     12U,
     kValleyRows,
     1U,
     {{ids::Id::kCharHerbalist, 4U, 11U, 3U, kDlgHerbalist}, {}, {}, {}},
     1U,
     {{7U, 10U, 2U, 1U}, {}}},
};

// Starting tile per map (the tile the party stands on when a chapter opens).
constexpr uint8_t kMapStartX[] = {7U, 7U, 7U, 5U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U};
constexpr uint8_t kMapStartY[] = {4U, 1U, 1U, 1U, 4U, 1U, 4U, 4U, 4U, 4U, 4U, 4U, 4U, 4U, 4U};

// --- table shape guards -----------------------------------------------------
// The tables are deliberately declared without a bound so these asserts can
// compare their real length against the count enum. A table that is one row
// short would otherwise be quietly zero-filled, handing the UI an entry whose
// ids are all zero; a map whose rows do not match its declared size would make
// MapTileAt walk off the end of a string literal. Both are compile errors here
// instead of crashes on the device.
constexpr uint32_t TextLength(const char* text) {
    uint32_t length = 0U;
    while (text != nullptr && text[length] != '\0') {
        ++length;
    }
    return length;
}

constexpr bool MapRowsMatchDeclaredSize() {
    for (uint8_t map = 0U; map < kMapCount; ++map) {
        if (kMaps[map].rows == nullptr) {
            return false;
        }
        for (uint8_t row = 0U; row < kMaps[map].height; ++row) {
            if (TextLength(kMaps[map].rows[row]) != kMaps[map].width) {
                return false;
            }
        }
    }
    return true;
}

static_assert(sizeof(kSkills) / sizeof(kSkills[0]) == kSkCount, "skill table length mismatch");
static_assert(sizeof(kCharacters) / sizeof(kCharacters[0]) == kCharacterCount,
              "character table length mismatch");
static_assert(sizeof(kItems) / sizeof(kItems[0]) == kItemIdxCount, "item table length mismatch");
static_assert(sizeof(kEnemies) / sizeof(kEnemies[0]) == kFoeCount, "enemy table length mismatch");
static_assert(sizeof(kBattles) / sizeof(kBattles[0]) == kBattleCount, "battle table length mismatch");
static_assert(sizeof(kDialogue) / sizeof(kDialogue[0]) == kDialogueCount,
              "dialogue table length mismatch");
static_assert(sizeof(kMaps) / sizeof(kMaps[0]) == kMapCount, "map table length mismatch");
static_assert(sizeof(kMapStartX) / sizeof(kMapStartX[0]) == kMapCount, "map start X length mismatch");
static_assert(sizeof(kMapStartY) / sizeof(kMapStartY[0]) == kMapCount, "map start Y length mismatch");
static_assert(MapRowsMatchDeclaredSize(), "a map's rows do not match its declared width/height");

// Per-entry counts must fit the fixed arrays the engine indexes them with.
// The battle and map code guards its cursors, but a content row that declares
// more skills/foes/npcs than the array holds is an out-of-bounds read waiting
// to happen, so it is rejected here instead.
constexpr bool EntryCountsFitEngineArrays() {
    for (uint8_t index = 0U; index < kCharacterCount; ++index) {
        if (kCharacters[index].skill_count > kMaxCharacterSkills) {
            return false;
        }
    }
    for (uint8_t index = 0U; index < kFoeCount; ++index) {
        if (kEnemies[index].skill_count > kMaxEnemySkills) {
            return false;
        }
    }
    for (uint8_t index = 0U; index < kBattleCount; ++index) {
        if (kBattles[index].enemy_count > kMaxBattleEnemies) {
            return false;
        }
    }
    for (uint8_t index = 0U; index < kMapCount; ++index) {
        if (kMaps[index].npc_count > kMaxMapNpcs || kMaps[index].trigger_count > kMaxMapTriggers) {
            return false;
        }
    }
    return true;
}

static_assert(EntryCountsFitEngineArrays(), "a content row declares more entries than the engine array holds");

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
    {StepKind::kExplore, kMapShu},
    {StepKind::kDialogue, kDlgShu},
    {StepKind::kGrant, kIdxShuTalisman, 2U},
    {StepKind::kBattle, kBattleTrial},
    // The buried blades under 蜀山.
    {StepKind::kExplore, kMapSwordTomb},
    {StepKind::kDialogue, kDlgSwordTomb},
    {StepKind::kBattle, kBattleSwordTomb},
    {StepKind::kDialogue, kDlgSwordAfter},
    {StepKind::kGrant, kIdxLingzhi, 2U},
    {StepKind::kExplore, kMapTower},
    {StepKind::kDialogue, kDlgTower},
    {StepKind::kBattle, kBattleTower},
    {StepKind::kDialogue, kDlgTowerTop},
    {StepKind::kBattle, kBattlePriests},
    // Act 4: the fall of the tower and the road south.
    {StepKind::kDialogue, kDlgTowerFall},
    {StepKind::kFlag, kBitYunyangGone},
    {StepKind::kExplore, kMapBaihe},
    {StepKind::kDialogue, kDlgBaihe},
    {StepKind::kGrant, kIdxHerb, 3U},
    {StepKind::kBattle, kBattleBaihe},
    {StepKind::kExplore, kMapMiao},
    {StepKind::kDialogue, kDlgAnu},
    {StepKind::kBattle, kBattleMiao},
    {StepKind::kDialogue, kDlgAnuAfter},
    {StepKind::kGrant, kIdxOxhorn, 2U},
    // The mist ravine between 苗疆 and the grove.
    {StepKind::kExplore, kMapValley},
    {StepKind::kDialogue, kDlgValley},
    {StepKind::kBattle, kBattleValley},
    {StepKind::kDialogue, kDlgValleyAfter},
    {StepKind::kGrant, kIdxHerb, 3U},
    // Act 5: the grove, the temple, and the showdown.
    {StepKind::kExplore, kMapGrove},
    {StepKind::kDialogue, kDlgGrove},
    {StepKind::kBattle, kBattleGrove},
    {StepKind::kExplore, kMapTemple},
    {StepKind::kDialogue, kDlgTemple},
    {StepKind::kBattle, kBattleGuardian},
    {StepKind::kDialogue, kDlgTempleAfter},
    {StepKind::kGrant, kIdxJade, 1U},
    {StepKind::kExplore, kMapAltar},
    {StepKind::kDialogue, kDlgAltar},
    {StepKind::kBattle, kBattleOverlord},
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
// ---------------------------------------------------------------------------
// Elements (五灵) and skill unlocks
//
// Kept as small parallel tables keyed by id instead of extra columns in the
// content rows: the main tables stay readable, and anything not authored here
// is simply neutral.
// ---------------------------------------------------------------------------

// Damage type per skill, in SkillId order.
constexpr uint8_t kSkillElements[] = {
    kElemNone,     // 御剑术
    kElemNone,     // 万剑诀
    kElemNone,     // 剑神
    kElemThunder,  // 五雷咒
    kElemNone,     // 观音咒
    kElemWater,    // 冰咆哮
    kElemNone,     // 疗伤术
    kElemNone,     // 金刚咒
    kElemWind,     // 风卷残云
    kElemNone,     // 噬魂咒
    kElemPoison,   // 蚀骨毒雾
    kElemFire,     // 炎杀
    kElemNone,     // 万剑归宗
    kElemWater,    // 水龙吟
    kElemFire,     // 红莲剑
    kElemPoison,   // 蛊毒
    kElemWater,    // 灵蛇咒
    kElemNone,     // 回春咒
    kElemPoison,   // 万蛊噬天
    kElemWater,    // 水龙潮
};
static_assert(sizeof(kSkillElements) / sizeof(kSkillElements[0]) == static_cast<uint32_t>(kSkCount),
              "kSkillElements must list every skill in order");

struct ElementResist final {
    uint8_t id;
    int8_t resist[kElemCount];  // indexed by kElem*; slot 0 (none) is unused
};

// Only the foes whose 五灵 identity matters are listed; the rest stay neutral.
// This is also what replaces hand-tuned HP when a boss should be hard: 蛛后
// resists water and poison, so 水龙吟 and 蛊毒 stop being the answer for it.
constexpr ElementResist kEnemyResist[] = {
    {kFoeWolf, {0, -20, 20, 0, 0, 0}},          // 蛇妖 — fears fire
    {kFoeGhost, {0, 0, 30, -20, 0, 0}},         // 水妖 — born of water, hates thunder
    {kFoeDemon, {0, 20, 10, -15, 0, 20}},       // 拜月教主
    {kFoeTreant, {0, -25, 15, 0, 10, 0}},       // 千年树精
    {kFoeWasp, {0, 0, 0, -20, 20, 30}},         // 妖蜂 — venomous, hates wind
    {kFoeCultist, {0, 0, 0, -15, 0, 0}},        // 拜月教徒
    {kFoeSerpent, {0, -20, 20, 0, 0, 0}},       // 妖蛇
    {kFoeGolem, {0, 20, 10, 20, -25, 0}},       // 石魔 — stone, but wind cracks it
    {kFoePriest, {0, 0, 0, -20, 0, 10}},        // 拜月祭司
    {kFoeOverlord, {0, 15, 10, -20, 0, 20}},    // 拜月教主·真身
    {kFoeSpider, {0, -20, 0, 0, 0, 40}},        // 毒蛛
    {kFoeShaman, {0, 0, 0, -15, 0, 30}},        // 蛊师
    {kFoeWaterBeast, {0, 0, 40, -25, 0, 0}},    // 水魔兽
    {kFoeSwordSpirit, {0, -15, 0, 0, 0, 0}},    // 剑灵
    {kFoeSpiderQueen, {0, -25, 10, 0, 0, 40}},  // 蛛后
};

// The party's own affinities: 赵灵儿 is a water spirit, 阿奴 was raised on 蛊.
constexpr ElementResist kCharacterResist[] = {
    {kCharXiao, {0, 0, 0, 0, 0, 0}},
    {kCharLingxi, {0, -20, 30, 0, 0, 0}},
    {kCharYunyang, {0, 0, 0, 0, 0, 0}},
    {kCharAnu, {0, 0, 0, 0, 0, 40}},
};

// Level at which each hero's four skills open, in CharacterDef::skills order.
// The first two arrive almost immediately on purpose: the content was balanced
// with a full kit at level 1, so only the third and the ultimate are held back -
// which is what "no ultimates at level 1" actually asks for.
constexpr uint8_t kSkillUnlockLevel[kCharacterCount][kMaxCharacterSkills] = {
    {1U, 2U, 6U, 13U},  // 李小遥 — 御剑术 / 万剑诀 / 剑神 / 万剑归宗
    {1U, 2U, 5U, 11U},  // 赵灵儿 — 五雷咒 / 观音咒 / 冰咆哮 / 水龙吟
    {1U, 2U, 6U, 12U},  // 林月如 — 疗伤术 / 金刚咒 / 风卷残云 / 红莲剑
    {1U, 2U, 5U, 11U},  // 阿奴 — 蛊毒 / 灵蛇咒 / 回春咒 / 万蛊噬天
};

int16_t LookupResist(const ElementResist* table, uint32_t count, uint8_t id, uint8_t element) {
    if (element == kElemNone || element >= kElemCount) {
        return 0;
    }
    for (uint32_t index = 0U; index < count; ++index) {
        if (table[index].id == id) {
            return table[index].resist[element];
        }
    }
    return 0;
}

const CharacterDef& Character(uint8_t id) { return kCharacters[id < kCharacterCount ? id : 0U]; }

uint8_t SkillElement(uint8_t skill_id) {
    return skill_id < static_cast<uint8_t>(kSkCount) ? kSkillElements[skill_id] : kElemNone;
}

int16_t EnemyElementResist(uint8_t enemy_id, uint8_t element) {
    return LookupResist(kEnemyResist, static_cast<uint32_t>(sizeof(kEnemyResist) / sizeof(kEnemyResist[0])),
                        enemy_id, element);
}

int16_t CharacterElementResist(uint8_t character, uint8_t element) {
    return LookupResist(kCharacterResist,
                        static_cast<uint32_t>(sizeof(kCharacterResist) / sizeof(kCharacterResist[0])),
                        character, element);
}

uint8_t SkillUnlockedCount(uint8_t character, uint8_t level) {
    if (character >= kCharacterCount) {
        return 0U;
    }
    uint8_t count = 0U;
    for (uint8_t row = 0U; row < kMaxCharacterSkills; ++row) {
        if (kSkillUnlockLevel[character][row] <= level) {
            ++count;
        }
    }
    return count;
}

uint8_t SkillRowSkill(uint8_t character, uint8_t level, uint8_t row) {
    if (character >= kCharacterCount || row >= kMaxCharacterSkills) {
        return kNoSkill;
    }
    if (kSkillUnlockLevel[character][row] > level) {
        return kNoSkill;
    }
    const CharacterDef& def = Character(character);
    return row < def.skill_count ? def.skills[row] : kNoSkill;
}

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
    const bool trust = (flags & kFlagTrust) != 0U;
    if (relic && mercy) {
        return kDlgEndGood;
    }
    if (bold || courtesy || trust) {
        return kDlgEndMid;
    }
    return kDlgEndBad;
}

}  // namespace pal
