// SPDX-License-Identifier: Apache-2.0
// Host-synth audio for 仙剑奇侠传.
//
// Sound effects are tone recipes authored in `audio/sfx.json`; the build turns
// that manifest into build/generated/pal_sfx_profiles.hpp and the ToneSequencer
// plays them through the Host mixer. The background tracks are the same kind of
// recipe played one note at a time, so the whole soundtrack costs no assets and
// no Bundle space - the device is what it sounds like, not what it plays.
//
// When the Host reports no audio capability (or the Bundle never asked for
// `audio.output`) the director degrades to a silent no-op: every call stays
// legal, so scenes never need a capability check of their own.

#ifndef PAL_AUDIO_HPP
#define PAL_AUDIO_HPP

#include <stdint.h>

#include "sdk/micropixel.hpp"
#include "sdk/tone_sequencer.hpp"

#include "pal_sfx_profiles.hpp"

namespace pal {

// One-shot effects, one per `effects` entry in audio/sfx.json. The order has to
// match kProfiles[] in pal_audio.cpp.
enum class SfxId : uint8_t {
    kCursor = 0U,  // menu / list focus moved
    kConfirm,      // a menu entry was taken
    kCancel,       // backed out
    kStep,         // one exploration tile
    kItem,         // consumable used
    kSave,         // 已记录天机
    kHeal,         // recovery magic landed
    kChapter,      // a chapter beat advanced
    kAttack,       // physical strike
    kHurt,         // the party took a hit
    kGuard,        // 防御
    kFoeDown,      // an enemy fell
    kPoison,       // 毒发
    kSkill,        // 术法 cast
    kEncounter,    // a battle opened
    kVictory,      // 战斗胜利
    kLevelUp,      // 境界提升
    kDefeat,       // 力竭倒地
    kCount,
};

// Looping background tracks, one per scene family.
enum class BgmId : uint8_t {
    kNone = 0U,
    kTitle,
    kField,   // towns, roads, forests
    kTense,   // 锁妖塔 / 毒瘴谷
    kBattle,
};

class AudioDirector final {
   public:
    AudioDirector(micropixel::Audio audio, bool enabled);

    [[nodiscard]] bool enabled() const { return tones_.enabled(); }

    void PlaySfx(SfxId id, uint8_t gain = 255U);

    // Selecting the track that is already playing is a no-op, so scenes can call
    // this unconditionally from Enter(). kNone stops the music.
    void SetBgm(BgmId id);
    void StopBgm() { SetBgm(BgmId::kNone); }
    [[nodiscard]] BgmId bgm() const { return bgm_; }

    // Drains the pending note delays and steps the background melody. Call once
    // per frame with the tick delta.
    void Advance(micropixel::Duration delta);

   private:
    micropixel::ToneSequencer<8U> tones_;
    BgmId bgm_{BgmId::kNone};
    uint32_t note_index_{};
    uint64_t note_remaining_us_{};
};

}  // namespace pal

#endif  // PAL_AUDIO_HPP
