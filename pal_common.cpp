// SPDX-License-Identifier: Apache-2.0
// Responsive layout for every screen.

#include "pal_common.hpp"

#include "sdk/math.hpp"

namespace pal {

namespace math = micropixel::math;

GameLayout BuildLayout(const micropixel::RendererInfo& info) {
    GameLayout layout{};
    layout.screen = info.safe_area();
    const int32_t w = layout.screen.width;
    const int32_t h = layout.screen.height;
    const int32_t x = layout.screen.x;
    const int32_t y = layout.screen.y;
    layout.compact = w < 430 || h < 470;
    const int32_t pad = math::Clamp<int32_t>(math::Min(w, h) / 64, 4, 8);

    const int32_t header_h = math::Clamp<int32_t>(h * 6 / 100, 18, 28);
    layout.header = {x, y, w, header_h};

    // The footer carries the exploration pad, so give it real estate: the pad
    // ends up thumb-sized instead of a row of small targets.
    const int32_t footer_h = math::Clamp<int32_t>(h * 54 / 100, 104, 200);
    layout.footer = {x, y + h - footer_h, w, footer_h};
    const int32_t body_y = y + header_h + 2;
    layout.body = {x, body_y, w, math::Max(y + h - footer_h - body_y - 2, 40)};

    // --- exploration: viewport plus a diamond pad and a menu button ----------
    layout.stage = layout.body;
    const int32_t cell =
        math::Min(math::Max((footer_h - pad * 4) / 3, 20), math::Max((w - pad * 8) / 3, 20));
    const int32_t dpad_h = cell * 3 + pad * 2;
    const int32_t dpad_w = dpad_h;
    // Centred and pushed to the very bottom of the screen: that is where a
    // thumb rests, and it buys the largest possible pad. The four arrows are
    // laid out from this origin below.
    const int32_t dpad_x = x + (w - dpad_w) / 2;
    const int32_t dpad_y = layout.footer.y + math::Max(footer_h - dpad_h - pad, pad);
    layout.dpad[0] = {dpad_x + cell + pad, dpad_y, cell, cell};                       // up
    layout.dpad[1] = {dpad_x, dpad_y + cell + pad, cell, cell};                       // left
    layout.dpad[2] = {dpad_x + (cell + pad) * 2, dpad_y + cell + pad, cell, cell};     // right
    layout.dpad[3] = {dpad_x + cell + pad, dpad_y + (cell + pad) * 2, cell, cell};     // down
    // The status button lives in the margin the centred pad leaves free.
    const int32_t margin_w = (w - dpad_w) / 2;
    const int32_t menu_w = math::Clamp<int32_t>(margin_w - pad * 2, 44, 128);
    const int32_t menu_h = math::Clamp<int32_t>(footer_h / 3, 26, 44);
    layout.menu_button = {x + w - pad - menu_w, layout.footer.y + (footer_h - menu_h) / 2, menu_w, menu_h};
    layout.list = layout.body;

    // --- dialogue: a big box over the bottom of the screen, the speaker
    //     standing just above it so the text keeps the full box width --------
    const int32_t box_h = math::Clamp<int32_t>(h * 34 / 100, 104, 200);
    layout.dialogue = {x + 3, y + h - box_h - 3, w - 6, box_h};
    const int32_t portrait_size = math::Min<int32_t>(box_h * 62 / 100, 96);
    layout.portrait = {layout.dialogue.x + 10, layout.dialogue.y - portrait_size + 10, portrait_size, portrait_size};
    layout.dialogue_text = {layout.dialogue.x + 10, layout.dialogue.y + 8, layout.dialogue.width - 20, box_h - 36};
    const int32_t choice_h = math::Clamp<int32_t>(box_h / 3, 32, 68);
    layout.choices = {layout.dialogue.x + 8, layout.dialogue.y + box_h - choice_h - 6, layout.dialogue.width - 16,
                      choice_h};

    // --- battle: foes on top, the party below, commands in the footer --------
    const int32_t stage_h = layout.body.height;
    layout.enemy_area = {x, layout.body.y, w, stage_h * 46 / 100};
    layout.party_area = {x, layout.enemy_area.y + layout.enemy_area.height, w,
                         math::Max(stage_h - layout.enemy_area.height, 30)};
    const int32_t msg_h = math::Clamp<int32_t>(footer_h * 38 / 100, 32, 62);
    layout.message = {x, layout.footer.y, w, msg_h};
    layout.command = {x, layout.message.y + layout.message.height, w, footer_h - msg_h};

    constexpr int32_t kCols = 3;
    constexpr int32_t kRows = 2;
    const int32_t gap = math::Max(pad - 2, 3);
    const int32_t cw = math::Max((layout.command.width - gap * (kCols + 1)) / kCols, 20);
    const int32_t ch = math::Max((layout.command.height - gap * (kRows + 1)) / kRows, 18);
    for (int32_t row = 0; row < kRows; ++row) {
        for (int32_t col = 0; col < kCols; ++col) {
            layout.commands[static_cast<uint32_t>(row * kCols + col)] = {
                layout.command.x + gap + col * (cw + gap), layout.command.y + gap + row * (ch + gap), cw, ch};
        }
    }

    return layout;
}

ids::Id ContextMapNameId(const GameContext& context) { return Map(context.world.map).name; }

}  // namespace pal
