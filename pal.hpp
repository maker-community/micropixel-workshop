// SPDX-License-Identifier: Apache-2.0
#ifndef PAL_HPP
#define PAL_HPP

// Umbrella header for 仙剑奇侠传. Keep this file dependency-free so main.cpp stays
// a one-liner, matching the official guest/apps/* layout.

#include <stdint.h>

namespace pal {

inline constexpr uint32_t kSaveVersion = 1U;

// Runs the whole application: builds the retained scene, wires the event loop
// and never returns until the Host stops the app.
int PalAppMain();

}  // namespace pal

#endif  // PAL_HPP
