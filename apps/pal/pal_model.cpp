// SPDX-License-Identifier: MIT
// Party growth curves, bag bookkeeping and the versioned save blob.

#include "pal_model.hpp"

#include "pal.hpp"

namespace pal {
namespace {

// --- save record layout -----------------------------------------------------
// Every field is written as a little-endian u32 so the blob is trivially
// forward/backward compatible: new fields go at the end and the version guard
// (pal::kSaveVersion) decides whether an old blob is migrated or discarded.

class BlobWriter final {
   public:
    BlobWriter(uint8_t* out, uint32_t capacity) : out_(out), capacity_(capacity) {}

    bool U32(uint32_t value) {
        if (at_ + 4U > capacity_) {
            return false;
        }
        out_[at_++] = static_cast<uint8_t>(value & 0xFFU);
        out_[at_++] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
        out_[at_++] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
        out_[at_++] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
        return true;
    }

    [[nodiscard]] uint32_t written() const { return at_; }

   private:
    uint8_t* out_{};
    uint32_t capacity_{};
    uint32_t at_{};
};

class BlobReader final {
   public:
    BlobReader(const uint8_t* data, uint32_t size) : data_(data), size_(size) {}

    uint32_t U32() {
        if (at_ + 4U > size_) {
            failed_ = true;
            return 0U;
        }
        const uint32_t value = static_cast<uint32_t>(data_[at_]) | (static_cast<uint32_t>(data_[at_ + 1U]) << 8U) |
                               (static_cast<uint32_t>(data_[at_ + 2U]) << 16U) |
                               (static_cast<uint32_t>(data_[at_ + 3U]) << 24U);
        at_ += 4U;
        return value;
    }

    [[nodiscard]] bool failed() const { return failed_; }

   private:
    const uint8_t* data_{};
    uint32_t size_{};
    uint32_t at_{};
    bool failed_{};
};

inline constexpr uint8_t kArmourBonusPerLevel = 0U;

}  // namespace

// --- derived stats ----------------------------------------------------------

uint16_t MemberMaxHp(const PartyMember& member) {
    if (member.character == kEmptySlot) {
        return 0U;
    }
    const CharacterDef& def = Character(member.character);
    return static_cast<uint16_t>(def.base_hp + def.growth_hp * (member.level - 1U));
}

uint16_t MemberMaxMp(const PartyMember& member) {
    if (member.character == kEmptySlot) {
        return 0U;
    }
    const CharacterDef& def = Character(member.character);
    return static_cast<uint16_t>(def.base_mp + def.growth_mp * (member.level - 1U));
}

uint8_t MemberAttack(const PartyMember& member) {
    if (member.character == kEmptySlot) {
        return 0U;
    }
    const CharacterDef& def = Character(member.character);
    return static_cast<uint8_t>(def.base_atk + def.growth_atk * (member.level - 1U));
}

uint8_t MemberDefense(const PartyMember& member) {
    if (member.character == kEmptySlot) {
        return 0U;
    }
    const CharacterDef& def = Character(member.character);
    return static_cast<uint8_t>(def.base_def + def.growth_def * (member.level - 1U) + kArmourBonusPerLevel);
}

uint8_t MemberSpeed(const PartyMember& member) {
    if (member.character == kEmptySlot) {
        return 0U;
    }
    const CharacterDef& def = Character(member.character);
    return static_cast<uint8_t>(def.base_spd + def.growth_spd * (member.level - 1U));
}

void PartyRest(Progress& progress, uint32_t permille) {
    for (uint8_t index = 0U; index < progress.party_size && index < kMaxParty; ++index) {
        PartyMember& member = progress.party[index];
        if (member.character == kEmptySlot || member.hp == 0U) {
            continue;
        }
        const uint32_t max_hp = MemberMaxHp(member);
        const uint32_t max_mp = MemberMaxMp(member);
        member.hp = static_cast<uint16_t>(member.hp + (max_hp - member.hp) * permille / 1000U);
        member.mp = static_cast<uint16_t>(member.mp + (max_mp - member.mp) * permille / 1000U);
    }
}

uint16_t MemberXpThreshold(uint8_t level) {
    if (level <= 1U) {
        return 0U;
    }
    const uint32_t steps = static_cast<uint32_t>(level - 1U);
    return static_cast<uint16_t>(16U * steps * (steps + 1U));
}

ids::Id PortraitNameId(uint8_t portrait) {
    switch (portrait) {
        case 0U:
            return ids::Id::kCharXiao;
        case 1U:
            return ids::Id::kCharLingxi;
        case 2U:
            return ids::Id::kCharYunyang;
        case 3U:
            return ids::Id::kCharMaster;
        case 4U:
            return ids::Id::kCharMerchant;
        case 5U:
            return ids::Id::kCharDemon;
        case 6U:
            return ids::Id::kCharAnu;
        case 7U:
            return ids::Id::kCharQueen;
        default:
            return ids::Id::kUiEmpty;
    }
}

ids::Id CharacterNameId(uint8_t character) {
    return character < kCharacterCount ? Character(character).name : ids::Id::kUiEmpty;
}

ids::Id MemberNameId(const PartyMember& member) { return CharacterNameId(member.character); }

// --- party ------------------------------------------------------------------

void ProgressReset(Progress& progress) {
    progress = Progress{};
    progress.party_size = 1U;
    progress.party[0].character = kCharXiao;
    progress.party[0].level = 1U;
    progress.party[0].hp = MemberMaxHp(progress.party[0]);
    progress.party[0].mp = MemberMaxMp(progress.party[0]);
    progress.party[0].xp = 0U;
    (void)BagAdd(progress, kIdxPillHp, 3U);
    (void)BagAdd(progress, kIdxPillMp, 2U);
    progress.map_id = kMapVillage;
    progress.player_x = static_cast<int16_t>(MapStartX(kMapVillage));
    progress.player_y = static_cast<int16_t>(MapStartY(kMapVillage));
}

int8_t PartyFind(const Progress& progress, uint8_t character) {
    for (uint8_t index = 0U; index < progress.party_size && index < kMaxParty; ++index) {
        if (progress.party[index].character == character) {
            return static_cast<int8_t>(index);
        }
    }
    return -1;
}

bool PartyAdd(Progress& progress, uint8_t character) {
    if (PartyFind(progress, character) >= 0) {
        return true;
    }
    if (progress.party_size >= kMaxParty) {
        return false;
    }
    // A late recruit arrives at the party's average level, otherwise 阿奴 would
    // walk into the final act at level 1 and be dead weight.
    uint32_t level_sum = 0U;
    uint32_t members = 0U;
    for (uint8_t index = 0U; index < progress.party_size && index < kMaxParty; ++index) {
        level_sum += progress.party[index].level;
        ++members;
    }
    const uint8_t join_level = members == 0U ? 1U : static_cast<uint8_t>(level_sum / members);
    PartyMember& member = progress.party[progress.party_size];
    member = PartyMember{};
    member.character = character;
    member.level = join_level < 1U ? 1U : join_level;
    member.hp = MemberMaxHp(member);
    member.mp = MemberMaxMp(member);
    member.xp = MemberXpThreshold(member.level);
    ++progress.party_size;
    return true;
}

bool PartyRemove(Progress& progress, uint8_t character) {
    const int8_t found = PartyFind(progress, character);
    if (found < 0) {
        return false;
    }
    for (uint8_t index = static_cast<uint8_t>(found); index + 1U < progress.party_size && index + 1U < kMaxParty;
         ++index) {
        progress.party[index] = progress.party[index + 1U];
    }
    --progress.party_size;
    progress.party[progress.party_size] = PartyMember{};
    return true;
}

void PartySyncFromFlags(Progress& progress) {
    if ((progress.flags & kFlagYunyangGone) != 0U) {
        (void)PartyRemove(progress, kCharYunyang);
    }
    if ((progress.flags & kFlagLingxi) != 0U) {
        (void)PartyAdd(progress, kCharLingxi);
    }
    if ((progress.flags & kFlagYunyang) != 0U && (progress.flags & kFlagYunyangGone) == 0U) {
        (void)PartyAdd(progress, kCharYunyang);
    }
    if ((progress.flags & kFlagAnu) != 0U) {
        (void)PartyAdd(progress, kCharAnu);
    }
}

uint32_t PartyGrantXp(Progress& progress, uint16_t xp) {
    uint32_t levels_gained = 0U;
    for (uint8_t index = 0U; index < progress.party_size && index < kMaxParty; ++index) {
        PartyMember& member = progress.party[index];
        if (member.character == kEmptySlot) {
            continue;
        }
        member.xp = static_cast<uint16_t>(member.xp + xp);
        while (member.level < kMaxLevel && member.xp >= MemberXpThreshold(static_cast<uint8_t>(member.level + 1U))) {
            ++member.level;
            ++levels_gained;
            // A level-up tops the member back to full, like every classic RPG.
            member.hp = MemberMaxHp(member);
            member.mp = MemberMaxMp(member);
        }
    }
    return levels_gained;
}

void PartyRestore(Progress& progress) {
    for (uint8_t index = 0U; index < progress.party_size && index < kMaxParty; ++index) {
        PartyMember& member = progress.party[index];
        if (member.character == kEmptySlot) {
            continue;
        }
        member.hp = MemberMaxHp(member);
        member.mp = MemberMaxMp(member);
        member.poisoned = false;
    }
}

void PartyClearPoison(Progress& progress) {
    for (uint8_t index = 0U; index < progress.party_size && index < kMaxParty; ++index) {
        progress.party[index].poisoned = false;
    }
}

bool PartyWiped(const Progress& progress) {
    bool any = false;
    for (uint8_t index = 0U; index < progress.party_size && index < kMaxParty; ++index) {
        if (progress.party[index].character == kEmptySlot) {
            continue;
        }
        any = true;
        if (progress.party[index].hp > 0U) {
            return false;
        }
    }
    return any;
}

uint16_t PartyTotalHp(const Progress& progress) {
    uint32_t total = 0U;
    for (uint8_t index = 0U; index < progress.party_size && index < kMaxParty; ++index) {
        total += progress.party[index].hp;
    }
    return static_cast<uint16_t>(total);
}

// --- bag --------------------------------------------------------------------

uint16_t BagCount(const Progress& progress, uint8_t item) {
    for (uint8_t index = 0U; index < progress.bag_size && index < kMaxBag; ++index) {
        if (progress.bag[index].item == item) {
            return progress.bag[index].count;
        }
    }
    return 0U;
}

bool BagHas(const Progress& progress, uint8_t item) { return BagCount(progress, item) > 0U; }

bool BagAdd(Progress& progress, uint8_t item, uint16_t count) {
    if (item == kItemNone || count == 0U) {
        return false;
    }
    for (uint8_t index = 0U; index < progress.bag_size && index < kMaxBag; ++index) {
        if (progress.bag[index].item == item) {
            progress.bag[index].count = static_cast<uint16_t>(progress.bag[index].count + count);
            return true;
        }
    }
    if (progress.bag_size >= kMaxBag) {
        return false;
    }
    progress.bag[progress.bag_size].item = item;
    progress.bag[progress.bag_size].count = count;
    ++progress.bag_size;
    return true;
}

bool BagConsume(Progress& progress, uint8_t item, uint16_t count) {
    for (uint8_t index = 0U; index < progress.bag_size && index < kMaxBag; ++index) {
        BagSlot& slot = progress.bag[index];
        if (slot.item != item) {
            continue;
        }
        if (slot.count < count) {
            return false;
        }
        slot.count = static_cast<uint16_t>(slot.count - count);
        if (slot.count == 0U) {
            // Compact the bag so the UI never shows holes.
            for (uint8_t move = index; move + 1U < progress.bag_size; ++move) {
                progress.bag[move] = progress.bag[move + 1U];
            }
            --progress.bag_size;
            progress.bag[progress.bag_size] = BagSlot{};
        }
        return true;
    }
    return false;
}

// --- save -------------------------------------------------------------------

uint32_t ProgressSerialize(const Progress& progress, uint8_t* out, uint32_t capacity) {
    BlobWriter writer{out, capacity};
    bool ok = writer.U32(kSaveVersion);
    ok = writer.U32(progress.flags) && ok;
    ok = writer.U32(progress.script_index) && ok;
    ok = writer.U32(progress.gold) && ok;
    ok = writer.U32(progress.play_seconds) && ok;
    ok = writer.U32(progress.party_size) && ok;
    for (uint8_t index = 0U; index < kMaxParty; ++index) {
        const PartyMember& member = progress.party[index];
        ok = writer.U32(member.character) && ok;
        ok = writer.U32(member.level) && ok;
        ok = writer.U32(member.hp) && ok;
        ok = writer.U32(member.mp) && ok;
        ok = writer.U32(member.xp) && ok;
        ok = writer.U32(member.poisoned ? 1U : 0U) && ok;
    }
    ok = writer.U32(progress.bag_size) && ok;
    for (uint8_t index = 0U; index < kMaxBag; ++index) {
        ok = writer.U32(progress.bag[index].item) && ok;
        ok = writer.U32(progress.bag[index].count) && ok;
    }
    ok = writer.U32(progress.map_id) && ok;
    ok = writer.U32(static_cast<uint32_t>(progress.player_x)) && ok;
    ok = writer.U32(static_cast<uint32_t>(progress.player_y)) && ok;
    return ok ? writer.written() : 0U;
}

bool ProgressDeserialize(Progress& progress, const uint8_t* data, uint32_t size) {
    BlobReader reader{data, size};
    const uint32_t version = reader.U32();
    if (version != kSaveVersion && version != 1U && version != 2U) {
        return false;
    }
    Progress loaded{};
    loaded.flags = reader.U32();
    loaded.script_index = static_cast<uint8_t>(reader.U32());
    // Older saves predate the steps inserted before the 南诏 finale; apply each
    // version's shift in order.
    if (version == 1U) {
        if (loaded.script_index >= kSaveV1ActFourAt) {
            loaded.script_index = static_cast<uint8_t>(loaded.script_index + 20U);
        } else if (loaded.script_index >= kSaveV1ShuGrantAt) {
            loaded.script_index = static_cast<uint8_t>(loaded.script_index + 1U);
        }
    }
    if (version <= 2U) {
        if (loaded.script_index >= kSaveV2ValleyAt) {
            loaded.script_index = static_cast<uint8_t>(loaded.script_index + 10U);
        } else if (loaded.script_index >= kSaveV2SwordTombAt) {
            loaded.script_index = static_cast<uint8_t>(loaded.script_index + 5U);
        }
    }
    loaded.gold = reader.U32();
    loaded.play_seconds = reader.U32();
    loaded.party_size = static_cast<uint8_t>(reader.U32());
    for (uint8_t index = 0U; index < kMaxParty; ++index) {
        PartyMember& member = loaded.party[index];
        member.character = static_cast<uint8_t>(reader.U32());
        member.level = static_cast<uint8_t>(reader.U32());
        member.hp = static_cast<uint16_t>(reader.U32());
        member.mp = static_cast<uint16_t>(reader.U32());
        member.xp = static_cast<uint16_t>(reader.U32());
        member.poisoned = reader.U32() != 0U;
    }
    loaded.bag_size = static_cast<uint8_t>(reader.U32());
    for (uint8_t index = 0U; index < kMaxBag; ++index) {
        loaded.bag[index].item = static_cast<uint8_t>(reader.U32());
        loaded.bag[index].count = static_cast<uint16_t>(reader.U32());
    }
    loaded.map_id = static_cast<uint8_t>(reader.U32());
    loaded.player_x = static_cast<int16_t>(reader.U32());
    loaded.player_y = static_cast<int16_t>(reader.U32());
    if (reader.failed()) {
        return false;
    }
    // Sanitise anything the blob could have got wrong before it reaches the UI.
    if (loaded.party_size > kMaxParty) {
        loaded.party_size = kMaxParty;
    }
    if (loaded.bag_size > kMaxBag) {
        loaded.bag_size = kMaxBag;
    }
    if (loaded.map_id >= kMapCount) {
        loaded.map_id = kMapVillage;
    }
    if (loaded.script_index > ScriptCount()) {
        loaded.script_index = ScriptCount();
    }
    for (uint8_t index = 0U; index < loaded.party_size; ++index) {
        PartyMember& member = loaded.party[index];
        if (member.character != kEmptySlot && member.character >= kCharacterCount) {
            member.character = kEmptySlot;
        }
        if (member.level < 1U) {
            member.level = 1U;
        }
        if (member.level > kMaxLevel) {
            member.level = kMaxLevel;
        }
        if (member.hp > MemberMaxHp(member)) {
            member.hp = MemberMaxHp(member);
        }
        if (member.mp > MemberMaxMp(member)) {
            member.mp = MemberMaxMp(member);
        }
    }
    for (uint8_t index = 0U; index < loaded.bag_size; ++index) {
        if (loaded.bag[index].item >= kItemIdxCount) {
            loaded.bag[index].item = kItemNone;
        }
    }
    progress = loaded;
    return true;
}

bool ProgressHasSave(micropixel::KVStore store) {
    const auto size = store.GetBytesSize(kSaveKey);
    return size.has_value() && size.value() > 0U;
}

bool ProgressLoad(Progress& progress, micropixel::KVStore store) {
    const auto size = store.GetBytesSize(kSaveKey);
    if (!size.has_value() || size.value() == 0U || size.value() > 1024U) {
        return false;
    }
    uint8_t buffer[1024]{};
    const auto read = store.GetBytes(kSaveKey, buffer, sizeof(buffer));
    if (!read.has_value()) {
        return false;
    }
    return ProgressDeserialize(progress, buffer, read.value());
}

void ProgressStore(const Progress& progress, micropixel::KVStore store) {
    uint8_t buffer[1024]{};
    const uint32_t size = ProgressSerialize(progress, buffer, sizeof(buffer));
    if (size == 0U) {
        return;
    }
    (void)store.SetBytes(kSaveKey, buffer, size);
}

}  // namespace pal
