// SPDX-License-Identifier: MIT
// Grid movement and chapter exits.

#include "pal_world.hpp"

#include "sdk/math.hpp"

namespace pal {

void WorldEnter(WorldState& world, uint8_t map_id) {
    world = WorldState{};
    world.map = map_id < kMapCount ? map_id : 0U;
    world.x = static_cast<int16_t>(MapStartX(world.map));
    world.y = static_cast<int16_t>(MapStartY(world.map));
    world.from_x = world.x;
    world.from_y = world.y;
}

void WorldRestore(WorldState& world, uint8_t map_id, int16_t x, int16_t y) {
    world = WorldState{};
    world.map = map_id < kMapCount ? map_id : 0U;
    const int32_t clamped_x = micropixel::math::Clamp<int32_t>(x, 0, Map(world.map).width - 1);
    const int32_t clamped_y = micropixel::math::Clamp<int32_t>(y, 0, Map(world.map).height - 1);
    world.x = static_cast<int16_t>(clamped_x);
    world.y = static_cast<int16_t>(clamped_y);
    world.from_x = world.x;
    world.from_y = world.y;
}

bool WorldOnTrigger(const WorldState& world) {
    const MapDef& map = Map(world.map);
    for (uint8_t index = 0U; index < map.trigger_count; ++index) {
        const MapTriggerDef& trigger = map.triggers[index];
        if (world.x >= trigger.x && world.y >= trigger.y && world.x < trigger.x + trigger.width &&
            world.y < trigger.y + trigger.height) {
            return true;
        }
    }
    return false;
}

const MapNpcDef* WorldNpcAt(uint8_t map_id, int32_t x, int32_t y) {
    const MapDef& map = Map(map_id);
    for (uint8_t index = 0U; index < map.npc_count; ++index) {
        if (map.npcs[index].x == x && map.npcs[index].y == y) {
            return &map.npcs[index];
        }
    }
    return nullptr;
}

void WorldUpdate(WorldState& world, uint32_t delta_ms) {
    if (world.moving) {
        world.move_ms += delta_ms;
        if (world.move_ms >= kStepMs) {
            world.move_ms = kStepMs;
            world.moving = false;
        }
    }
    if (!world.moving && !world.completed && WorldOnTrigger(world)) {
        world.completed = true;
    }
}

WorldStepResult WorldStep(WorldState& world, int32_t dx, int32_t dy) {
    if (world.moving || (dx == 0 && dy == 0)) {
        return WorldStepResult::kNone;
    }
    if (dx != 0) {
        world.facing = dx < 0 ? kFaceLeft : kFaceRight;
    } else {
        world.facing = dy < 0 ? kFaceUp : kFaceDown;
    }
    const int32_t target_x = world.x + dx;
    const int32_t target_y = world.y + dy;
    if (const MapNpcDef* npc = WorldNpcAt(world.map, target_x, target_y)) {
        world.pending_dialogue = npc->dialogue;
        return WorldStepResult::kTalk;
    }
    if (!MapTileWalkable(world.map, target_x, target_y)) {
        return WorldStepResult::kBlocked;
    }
    world.from_x = world.x;
    world.from_y = world.y;
    world.x = static_cast<int16_t>(target_x);
    world.y = static_cast<int16_t>(target_y);
    world.move_ms = 0U;
    world.moving = true;
    return WorldStepResult::kMoved;
}

int32_t WorldRenderX16(const WorldState& world) {
    const int32_t target = static_cast<int32_t>(world.x) * 16;
    if (!world.moving) {
        return target;
    }
    const int32_t origin = static_cast<int32_t>(world.from_x) * 16;
    const int32_t elapsed = static_cast<int32_t>(world.move_ms);
    return origin + (target - origin) * elapsed / static_cast<int32_t>(kStepMs);
}

int32_t WorldRenderY16(const WorldState& world) {
    const int32_t target = static_cast<int32_t>(world.y) * 16;
    if (!world.moving) {
        return target;
    }
    const int32_t origin = static_cast<int32_t>(world.from_y) * 16;
    const int32_t elapsed = static_cast<int32_t>(world.move_ms);
    return origin + (target - origin) * elapsed / static_cast<int32_t>(kStepMs);
}

}  // namespace pal
