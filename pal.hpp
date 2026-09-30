// SPDX-License-Identifier: Apache-2.0
#ifndef PAL_HPP
#define PAL_HPP

// Umbrella header for 仙剑奇侠传. Keep this file dependency-free so main.cpp stays
// a one-liner, matching the official guest/apps/* layout.

#include <stdint.h>

namespace pal {

// v2 added the Act 4-5 chapters, 阿奴 and a 蜀山 gift; v3 the 剑冢 and 毒瘴谷
// chapters. Older saves resume with their script index shifted past the steps
// inserted before them (see ProgressDeserialize).
inline constexpr uint32_t kSaveVersion = 3U;
inline constexpr uint32_t kSaveV1ShuGrantAt = 27U;  // v1: one step inserted at 27
inline constexpr uint32_t kSaveV1ActFourAt = 33U;   // v1: twenty steps in total by 33
inline constexpr uint32_t kSaveV2SwordTombAt = 29U;  // v2: five steps inserted at 29
inline constexpr uint32_t kSaveV2ValleyAt = 45U;     // v2: five more by 45

// Runs the whole application: builds the retained scene, wires the event loop
// and never returns until the Host stops the app.
int PalAppMain();

}  // namespace pal

#endif  // PAL_HPP
