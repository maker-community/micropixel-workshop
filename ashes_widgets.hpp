// SPDX-License-Identifier: Apache-2.0
// Shared chrome and the procedural vector art.
//
// Every character, monster and tile is composed from rounded rectangles and
// fills in normalised 0..1000 coordinates, so the game ships no bitmap art and
// scales cleanly from a 320px square panel to a 720px landscape one.

#ifndef ASHES_WIDGETS_HPP
#define ASHES_WIDGETS_HPP

#include "ashes_common.hpp"

namespace ashes {
namespace widgets {

// Framed panel used by every screen.
void Panel(GameView& view, micropixel::Rect rect, micropixel::Color fill, micropixel::Color edge,
           uint32_t radius = 10U);

// Slim top bar with the current location and the party purse.
void Header(GameContext& context, ids::Id title, bool show_gold);

// 0 萧余烬, 1 灵汐, 2 云阳子, 3 青云子, 4 商队首领, 5 玄冥魔尊, 0xFF none.
void Portrait(GameView& view, micropixel::Rect area, uint8_t portrait, uint8_t emotion, bool highlight);

// Monsters: 0 妖狼, 1 山贼, 2 怨灵, 3 玄冥魔尊.
void EnemySprite(GameView& view, micropixel::Rect area, uint8_t sprite, bool alive, bool targeted);

// Overworld token for a party member.
void HeroToken(GameView& view, micropixel::Rect area, uint8_t character, uint8_t facing);

// One exploration tile.
void DrawTile(GameView& view, micropixel::Rect area, uint8_t tile);

void NameTag(GameView& view, micropixel::Rect area, const char* text, micropixel::Color color);
void HpMpBars(GameView& view, micropixel::Rect area, const Combatant& unit, bool show_mp);

// Sprite selector that matches a character id to its portrait art.
uint8_t PortraitForCharacter(uint8_t character);

}  // namespace widgets
}  // namespace ashes

#endif  // ASHES_WIDGETS_HPP
