// SPDX-License-Identifier: Apache-2.0
// See pal_audio.hpp. Every number here comes from the generated profile header,
// which the Build produces from audio/sfx.json; this file only decides *when* a
// recipe plays.

#include "pal_audio.hpp"

#include <span>

namespace pal {
namespace {

struct Profile final {
    const micropixel::ToneSpec* tones;
    uint32_t count;
};

// Indexed by SfxId - the static_assert below keeps the two in step.
constexpr Profile kProfiles[] = {
    {pal_sfx::kCursor, pal_sfx::kCursorCount},
    {pal_sfx::kConfirm, pal_sfx::kConfirmCount},
    {pal_sfx::kCancel, pal_sfx::kCancelCount},
    {pal_sfx::kStep, pal_sfx::kStepCount},
    {pal_sfx::kItem, pal_sfx::kItemCount},
    {pal_sfx::kSave, pal_sfx::kSaveCount},
    {pal_sfx::kHeal, pal_sfx::kHealCount},
    {pal_sfx::kChapter, pal_sfx::kChapterCount},
    {pal_sfx::kAttack, pal_sfx::kAttackCount},
    {pal_sfx::kHurt, pal_sfx::kHurtCount},
    {pal_sfx::kGuard, pal_sfx::kGuardCount},
    {pal_sfx::kFoeDown, pal_sfx::kFoeDownCount},
    {pal_sfx::kPoison, pal_sfx::kPoisonCount},
    {pal_sfx::kSkill, pal_sfx::kSkillCount},
    {pal_sfx::kEncounter, pal_sfx::kEncounterCount},
    {pal_sfx::kVictory, pal_sfx::kVictoryCount},
    {pal_sfx::kLevelUp, pal_sfx::kLevelUpCount},
    {pal_sfx::kDefeat, pal_sfx::kDefeatCount},
};
static_assert(sizeof(kProfiles) / sizeof(kProfiles[0]) == static_cast<size_t>(SfxId::kCount),
              "kProfiles must cover every SfxId in order");

struct Track final {
    const micropixel::ToneSpec* tones;
    uint32_t count;
    uint32_t interval_us;  // time between the start of consecutive notes
};

// Indexed by BgmId, with a silent slot for kNone so no arithmetic is needed.
// The notes are 145 ms long, so the interval is what sets the tempo: 400 ms is
// a slow 150 BPM eighth-note pulse, 200 ms drives the battle theme.
constexpr Track kTracks[] = {
    {nullptr, 0U, 0U},
    {pal_sfx::kBgmTitle, pal_sfx::kBgmTitleCount, 400000U},
    {pal_sfx::kBgmField, pal_sfx::kBgmFieldCount, 272727U},
    {pal_sfx::kBgmTense, pal_sfx::kBgmTenseCount, 352941U},
    {pal_sfx::kBgmBattle, pal_sfx::kBgmBattleCount, 200000U},
};
static_assert(sizeof(kTracks) / sizeof(kTracks[0]) == static_cast<size_t>(BgmId::kBattle) + 1U,
              "kTracks must have one entry per BgmId");

}  // namespace

AudioDirector::AudioDirector(micropixel::Audio audio, bool enabled) : tones_(audio, enabled) {}

void AudioDirector::PlaySfx(SfxId id, uint8_t gain) {
    if (static_cast<uint32_t>(id) >= static_cast<uint32_t>(SfxId::kCount)) {
        return;
    }
    const Profile& profile = kProfiles[static_cast<uint32_t>(id)];
    (void)tones_.Play(std::span<const micropixel::ToneSpec>(profile.tones, profile.count), gain);
}

void AudioDirector::SetBgm(BgmId id) {
    if (id == bgm_) {
        return;
    }
    bgm_ = id;
    // Drop the previous track's queued notes, otherwise the tail of the old
    // melody leaks over the first bars of the new one.
    tones_.Clear();
    note_index_ = 0U;
    note_remaining_us_ = 0U;
}

void AudioDirector::Advance(micropixel::Duration delta) {
    tones_.Advance(delta);

    if (bgm_ == BgmId::kNone || !tones_.enabled()) {
        return;
    }

    const uint64_t delta_us = delta.count_microseconds();
    if (delta_us < note_remaining_us_) {
        note_remaining_us_ -= delta_us;
        return;
    }

    const Track& melody = kTracks[static_cast<uint32_t>(bgm_)];
    (void)tones_.PlayNow(melody.tones[note_index_].ToTone());
    note_index_ = (note_index_ + 1U) % melody.count;
    // Carry the overshoot so a long frame cannot drift the tempo.
    const uint64_t overshoot_us = delta_us - note_remaining_us_;
    note_remaining_us_ = melody.interval_us > overshoot_us ? melody.interval_us - overshoot_us : 0U;
}

}  // namespace pal
