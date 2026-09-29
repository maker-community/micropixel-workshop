// SPDX-License-Identifier: Apache-2.0
#ifndef ASHES_HPP
#define ASHES_HPP

// Umbrella header for 仙剑奇侠传. Keep this file dependency-free so main.cpp stays
// a one-liner, matching the official guest/apps/* layout.

#include <stdint.h>

namespace ashes {

inline constexpr uint32_t kSaveVersion = 1U;

// Runs the whole application: builds the retained scene, wires the event loop
// and never returns until the Host stops the app.
int AshesAppMain();

}  // namespace ashes

#endif  // ASHES_HPP
