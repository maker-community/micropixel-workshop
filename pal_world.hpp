// SPDX-License-Identifier: Apache-2.0
// Grid exploration: where the party stands on the current map, tile-stepped
// movement with interpolation, NPC interaction and chapter exit triggers.

#ifndef PAL_WORLD_HPP
#define PAL_WORLD_HPP

#include <stdint.h>

#include "pal_content.hpp"
#include "pal_model.hpp"  // kEmptySlot sentinel

namespace pal {

// Milliseconds a single tile step takes to animate.
inline constexpr uint32_t kStepMs = 150U;

// Facing, also used to pick the overworld sprite orientation.
enum : uint8_t { kFaceDown = 0U, kFaceLeft = 1U, kFaceRight = 2U, kFaceUp = 3U };

struct WorldState final {
    uint8_t map{kMapVillage};
    int16_t x{};
    int16_t y{};
    int16_t from_x{};
    int16_t from_y{};
    uint32_t move_ms{};
    bool moving{};
    uint8_t facing{kFaceDown};
    bool completed{};                        // chapter exit reached
    uint8_t pending_dialogue{kEmptySlot};    // NPC dialogue requested by input
};

enum class WorldStepResult : uint8_t {
    kNone,     // a step is already animating
    kMoved,
    kBlocked,  // wall or off-map
    kTalk,     // the target tile holds an NPC; see pending_dialogue
};

void WorldEnter(WorldState& world, uint8_t map_id);
void WorldRestore(WorldState& world, uint8_t map_id, int16_t x, int16_t y);
void WorldUpdate(WorldState& world, uint32_t delta_ms);

WorldStepResult WorldStep(WorldState& world, int32_t dx, int32_t dy);

// Interpolated token position in 16ths of a tile, for smooth rendering.
int32_t WorldRenderX16(const WorldState& world);
int32_t WorldRenderY16(const WorldState& world);

// True when the standing tile is inside one of the map's exit triggers.
bool WorldOnTrigger(const WorldState& world);

// NPC occupying a tile, or nullptr.
const MapNpcDef* WorldNpcAt(uint8_t map_id, int32_t x, int32_t y);

}  // namespace pal

#endif  // PAL_WORLD_HPP
