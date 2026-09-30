// SPDX-License-Identifier: Apache-2.0
// Runtime party state, bag and the versioned KV save.
//
// The model layer never touches graphics: it only knows numbers and ids, which
// keeps the battle/dialogue rules independently testable and lets the save blob
// stay a flat, versioned byte record.

#ifndef PAL_MODEL_HPP
#define PAL_MODEL_HPP

#include <stdint.h>

#include "pal_content.hpp"
#include "sdk/micropixel.hpp"

namespace pal {

inline constexpr uint8_t kMaxParty = 3U;
inline constexpr uint8_t kMaxBag = 8U;
inline constexpr uint8_t kEmptySlot = 0xFFU;
inline constexpr uint8_t kMaxLevel = 40U;

struct PartyMember final {
    uint8_t character{kEmptySlot};  // character id, kEmptySlot when the slot is unused
    uint8_t level{1U};
    uint16_t hp{};
    uint16_t mp{};
    uint16_t xp{};  // total accumulated experience
    bool poisoned{};
};

struct BagSlot final {
    uint8_t item{kItemNone};
    uint16_t count{};
};

struct Progress final {
    uint32_t flags{};        // story flags, see pal_content.hpp
    uint8_t script_index{};  // position in the chapter script
    uint32_t gold{};
    uint32_t play_seconds{};
    uint8_t party_size{};  // occupied slots are packed at the front
    PartyMember party[kMaxParty]{};
    uint8_t bag_size{};
    BagSlot bag[kMaxBag]{};
    uint8_t map_id{};  // last exploration map, for a faithful resume
    int16_t player_x{};
    int16_t player_y{};
};

// --- derived stats ----------------------------------------------------------

uint16_t MemberMaxHp(const PartyMember& member);
uint16_t MemberMaxMp(const PartyMember& member);
uint8_t MemberAttack(const PartyMember& member);
uint8_t MemberDefense(const PartyMember& member);
uint8_t MemberSpeed(const PartyMember& member);
uint16_t MemberXpThreshold(uint8_t level);  // total xp required to hold `level`
ids::Id MemberNameId(const PartyMember& member);
// Localized name of a playable character id.
ids::Id CharacterNameId(uint8_t character);
// Localized name for a portrait selector (0..2 heroes, 3..5 story NPCs).
ids::Id PortraitNameId(uint8_t portrait);

// --- party ------------------------------------------------------------------

void ProgressReset(Progress& progress);
int8_t PartyFind(const Progress& progress, uint8_t character);
bool PartyAdd(Progress& progress, uint8_t character);
// Drops a member and packs the remaining slots; false when they were not in the party.
bool PartyRemove(Progress& progress, uint8_t character);
// Ensures every character whose flag is set is actually in the party (and every
// fallen one is gone). Called after dialogue so choices can recruit companions
// declaratively.
void PartySyncFromFlags(Progress& progress);
uint32_t PartyGrantXp(Progress& progress, uint16_t xp);  // returns levels gained
void PartyRestore(Progress& progress);                   // full heal and revive
// Restores `permille` of what everyone is missing, and revives nobody. The
// rest points between chapters hand out a breather, not a full heal, so items
// keep a job.
void PartyRest(Progress& progress, uint32_t permille);
void PartyClearPoison(Progress& progress);
bool PartyWiped(const Progress& progress);
uint16_t PartyTotalHp(const Progress& progress);

// --- bag --------------------------------------------------------------------

bool BagAdd(Progress& progress, uint8_t item, uint16_t count);
bool BagConsume(Progress& progress, uint8_t item, uint16_t count = 1U);
uint16_t BagCount(const Progress& progress, uint8_t item);
bool BagHas(const Progress& progress, uint8_t item);

// --- save -------------------------------------------------------------------

inline constexpr char kSaveKey[] = "save";

// Flat little-endian record; returns the number of bytes written.
uint32_t ProgressSerialize(const Progress& progress, uint8_t* out, uint32_t capacity);
bool ProgressDeserialize(Progress& progress, const uint8_t* data, uint32_t size);

bool ProgressHasSave(micropixel::KVStore store);
bool ProgressLoad(Progress& progress, micropixel::KVStore store);
void ProgressStore(const Progress& progress, micropixel::KVStore store);

}  // namespace pal

#endif  // PAL_MODEL_HPP
