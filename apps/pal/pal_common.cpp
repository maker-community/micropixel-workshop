// SPDX-License-Identifier: MIT
// Responsive layout for every screen.

#include "pal_common.hpp"

#include "sdk/math.hpp"

namespace pal {

namespace math = micropixel::math;

namespace {

// The pad only moves out of the footer when the ring can hold a comfortable
// target; on a rectangular Host window the safe area *is* the surface, so there
// is no ring and the footer pad of old is kept.
inline constexpr int32_t kRingPadMinThickness = 34;

struct RingMetrics final {
    int32_t center_x{};
    int32_t center_y{};
    int32_t inner{};          // distance from the centre where a button starts
    int32_t outer{};          // ...and where it ends, short of the panel rim
    int32_t thickness{};      // outer - inner: how deep a ring button is
    int32_t length{};         // tangential size that still clears the circle

    [[nodiscard]] bool usable() const { return thickness >= kRingPadMinThickness; }
};

// The safe area of a round panel is the inscribed square, so the ring's depth at
// an edge midpoint is (radius - half side). The four edge midpoints are the only
// places with room: the square's corners touch the circle.
RingMetrics MeasureRing(const micropixel::Rect& panel, const micropixel::Rect& screen) {
    RingMetrics ring{};
    ring.center_x = panel.center_x();
    ring.center_y = panel.center_y();
    const int32_t radius = math::Min(panel.width, panel.height) / 2;
    const int32_t half = math::Min(screen.width, screen.height) / 2;
    const int32_t available = math::Max(radius - half, 0);
    if (available < 24) {
        return ring;  // no ring worth using: thickness stays 0
    }
    // The two margins come out of the ring so a button clears both the rim and
    // the square instead of touching either.
    const int32_t margin = math::Clamp<int32_t>(available / 6, 6, 14);
    ring.outer = radius - margin;
    ring.inner = half + margin;
    ring.thickness = ring.outer - ring.inner;
    // The far corners of a button lie at (outer, length/2) from the centre and
    // must stay inside the circle.
    const int32_t clearance =
        static_cast<int32_t>(math::Sqrt(static_cast<float>(radius * radius - ring.outer * ring.outer)));
    // Even length keeps the button centred on the edge: an odd one is off by
    // half a pixel, which shows on a 1:1 panel.
    ring.length = math::Max(math::Min(clearance * 2, screen.width / 3), 44) / 2 * 2;
    return ring;
}

}  // namespace

GameLayout BuildLayout(const micropixel::RendererInfo& info) {
    GameLayout layout{};
    layout.panel = {0, 0, static_cast<int32_t>(info.width()), static_cast<int32_t>(info.height())};
    layout.screen = info.safe_area();
    const int32_t w = layout.screen.width;
    const int32_t h = layout.screen.height;
    const int32_t x = layout.screen.x;
    const int32_t y = layout.screen.y;
    layout.compact = w < 430 || h < 470;
    const int32_t pad = math::Clamp<int32_t>(math::Min(w, h) / 64, 4, 8);

    const int32_t header_h = math::Clamp<int32_t>(h * 6 / 100, 18, 28);
    layout.header = {x, y, w, header_h};

    // The ring owns the pad now, so the footer only carries the status button
    // (and the menu's own two) and the body takes every pixel it gives up.
    const RingMetrics ring = MeasureRing(layout.panel, layout.screen);
    layout.ring_pad = ring.usable();
    const int32_t footer_h = layout.ring_pad ? math::Clamp<int32_t>(h * 15 / 100, 36, 56)
                                             : math::Clamp<int32_t>(h * 54 / 100, 104, 200);
    layout.footer = {x, y + h - footer_h, w, footer_h};
    const int32_t body_y = y + header_h + 2;
    layout.body = {x, body_y, w, math::Max(y + h - footer_h - body_y - 2, 40)};

    // --- exploration: viewport plus the direction pad and a menu button ------
    // The pad is four keys on the panel rim on a round panel, or the old footer
    // cross where there is no ring to hold them.
    layout.stage = layout.body;
    if (layout.ring_pad) {
        // One pad key per edge midpoint, placed to match the direction it
        // walks: up on the top rim, left on the left rim, and so on.
        layout.dpad[0] = {ring.center_x - ring.length / 2, ring.center_y - ring.outer, ring.length,
                          ring.thickness};  // up
        layout.dpad[1] = {ring.center_x - ring.outer, ring.center_y - ring.length / 2, ring.thickness,
                          ring.length};  // left
        layout.dpad[2] = {ring.center_x + ring.inner, ring.center_y - ring.length / 2, ring.thickness,
                          ring.length};  // right
        layout.dpad[3] = {ring.center_x - ring.length / 2, ring.center_y + ring.inner, ring.length,
                          ring.thickness};  // down
        // The status button parks against the footer's right edge. The down key
        // owns the whole middle of the bottom rim, so a pill beside it reads as
        // one crowded clump: the corner leaves ~33 px of clearance and keeps the
        // two controls on separate bands.
        const int32_t menu_h = math::Max(footer_h - 10, 24);
        const int32_t menu_w = math::Max(menu_h * 7 / 4, 52);
        layout.menu_button = {x + w - pad - menu_w, layout.footer.y + (footer_h - menu_h) / 2, menu_w, menu_h};
    } else {
        const int32_t cell =
            math::Min(math::Max((footer_h - pad * 4) / 3, 20), math::Max((w - pad * 8) / 3, 20));
        const int32_t dpad_h = cell * 3 + pad * 2;
        const int32_t dpad_w = dpad_h;
        // Centred and pushed to the very bottom of the screen: that is where a
        // thumb rests, and it buys the largest possible pad. The four arrows are
        // laid out from this origin below.
        const int32_t dpad_x = x + (w - dpad_w) / 2;
        const int32_t dpad_y = layout.footer.y + math::Max(footer_h - dpad_h - pad, pad);
        layout.dpad[0] = {dpad_x + cell + pad, dpad_y, cell, cell};                    // up
        layout.dpad[1] = {dpad_x, dpad_y + cell + pad, cell, cell};                    // left
        layout.dpad[2] = {dpad_x + (cell + pad) * 2, dpad_y + cell + pad, cell, cell};  // right
        layout.dpad[3] = {dpad_x + cell + pad, dpad_y + (cell + pad) * 2, cell, cell};  // down
        // The status button lives in the margin the centred pad leaves free.
        const int32_t margin_w = (w - dpad_w) / 2;
        const int32_t menu_w = math::Clamp<int32_t>(margin_w - pad * 2, 44, 128);
        const int32_t menu_h = math::Clamp<int32_t>(footer_h / 3, 26, 44);
        layout.menu_button = {x + w - pad - menu_w, layout.footer.y + (footer_h - menu_h) / 2, menu_w, menu_h};
    }
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

    // --- battle: foes on top, the party below, a one-line message strip and a
    //     compact command panel pinned to the bottom. The exploration footer is
    //     far too tall for this: it left the foes and the party cards ~50px. ---
    const int32_t msg_h = math::Clamp<int32_t>(h * 8 / 100, 22, 34);
    const int32_t cmd_h = math::Clamp<int32_t>(h * 27 / 100, 72, 130);
    const int32_t battle_top = layout.header.y + layout.header.height + 2;
    const int32_t battle_stage_h = math::Max(y + h - cmd_h - msg_h - battle_top, 60);
    layout.enemy_area = {x, battle_top, w, battle_stage_h * 48 / 100};
    layout.party_area = {x, layout.enemy_area.y + layout.enemy_area.height, w,
                         math::Max(battle_stage_h - layout.enemy_area.height, 30)};
    layout.message = {x, y + h - cmd_h - msg_h, w, msg_h};
    layout.command = {x, layout.message.y + layout.message.height, w, cmd_h};

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
