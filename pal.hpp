// SPDX-License-Identifier: Apache-2.0
#ifndef PAL_HPP
#define PAL_HPP

// Umbrella header for 仙剑奇侠传. Keep this file dependency-free so main.cpp stays
// a one-liner, matching the official guest/apps/* layout.

#include <stdint.h>

namespace pal {

// v2 added the Act 4-5 chapters, 阿奴 and a 蜀山 gift. v1 saves resume with
// their script index shifted past the steps inserted before them.
inline constexpr uint32_t kSaveVersion = 2U;
inline constexpr uint32_t kSaveV1ShuGrantAt = 27U;  // one step inserted at 27
inline constexpr uint32_t kSaveV1ActFourAt = 33U;   // twenty steps in total by 33

// Runs the whole application: builds the retained scene, wires the event loop
// and never returns until the Host stops the app.
int PalAppMain();

}  // namespace pal

#endif  // PAL_HPP
