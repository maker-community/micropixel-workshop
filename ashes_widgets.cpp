// SPDX-License-Identifier: Apache-2.0
// Procedural vector art and the shared panels.

#include "ashes_widgets.hpp"

#include "sdk/math.hpp"

namespace ashes {
namespace widgets {
namespace {

// Normalised cell: x/y/w/h are 0..1000 fractions of `area`.
micropixel::Rect Cell(const micropixel::Rect& area, int32_t x, int32_t y, int32_t w, int32_t h) {
    return {area.x + area.width * x / 1000,
            area.y + area.height * y / 1000,
            micropixel::math::Max(area.width * w / 1000, 1),
            micropixel::math::Max(area.height * h / 1000, 1)};
}

struct FaceStyle final {
    micropixel::Color skin;
    micropixel::Color skin_shade;
    micropixel::Color hair;
    micropixel::Color hair_light;
    micropixel::Color eye;
    uint8_t ears{};    // 0 none, 1 fox, 2 horns
    bool beard{};
    bool young{};      // rounder face, bigger eyes
};

// Head, hair and expression. `emotion` is 0 calm, 1 angry, 2 sad, 3 glad.
void DrawHead(GameView& view, const micropixel::Rect& area, const FaceStyle& style, uint8_t emotion) {
    // back hair + optional appendages
    view.Round(Cell(area, 150, 90, 700, 700), style.hair, style.hair_light, 240U, 2U);
    if (style.ears == 1U) {
        view.Round(Cell(area, 120, 40, 180, 220), style.hair, style.hair_light, 90U, 2U);
        view.Round(Cell(area, 700, 40, 180, 220), style.hair, style.hair_light, 90U, 2U);
        view.Round(Cell(area, 165, 95, 90, 110), theme::kBlood.Lightened(90), style.hair_light, 45U, 1U);
        view.Round(Cell(area, 745, 95, 90, 110), theme::kBlood.Lightened(90), style.hair_light, 45U, 1U);
    }
    if (style.ears == 2U) {
        view.Round(Cell(area, 185, 20, 150, 240), theme::kBlood.Darkened(40), theme::kBlood, 60U, 2U);
        view.Round(Cell(area, 665, 20, 150, 240), theme::kBlood.Darkened(40), theme::kBlood, 60U, 2U);
    }

    // face
    const int32_t face_w = style.beard ? 500 : (style.young ? 520 : 480);
    view.Round(Cell(area, 500 - face_w / 2, 220, face_w, 560), style.skin, style.skin_shade, 200U, 1U);

    // hair cap and fringe
    view.Round(Cell(area, 190, 120, 620, 260), style.hair, style.hair_light, 190U, 2U);
    view.Round(Cell(area, 215, 250, 570, 170), style.hair, style.hair, 90U, 0U);
    if (!style.beard) {
        view.Round(Cell(area, 205, 230, 200, 240), style.hair, style.hair, 90U, 0U);
        view.Round(Cell(area, 595, 230, 200, 240), style.hair, style.hair, 90U, 0U);
    }

    // eyes: whites, irises, glints
    const int32_t eye_y = style.young ? 390 : 415;
    view.Round(Cell(area, 300, eye_y, 160, 130), micropixel::Color::White(), style.skin_shade, 60U, 0U);
    view.Round(Cell(area, 540, eye_y, 160, 130), micropixel::Color::White(), style.skin_shade, 60U, 0U);
    const int32_t iris_w = emotion == 1U ? 90 : 78;
    view.Round(Cell(area, 340, eye_y + 20, iris_w, 95), style.eye, style.eye, 40U, 0U);
    view.Round(Cell(area, 578, eye_y + 20, iris_w, 95), style.eye, style.eye, 40U, 0U);
    view.Fill(Cell(area, 352, eye_y + 30, 26, 30), micropixel::Color::White());
    view.Fill(Cell(area, 590, eye_y + 30, 26, 30), micropixel::Color::White());

    // brows follow the emotion
    int32_t brow_dy = 0;
    int32_t brow_inner = 0;
    if (emotion == 1U) {
        brow_dy = 22;
        brow_inner = 26;
    } else if (emotion == 2U) {
        brow_dy = -6;
        brow_inner = -30;
    }
    view.Round(Cell(area, 300, 350 + brow_dy + brow_inner, 165, 30), style.hair, style.hair, 14U, 0U);
    view.Round(Cell(area, 535, 350 + brow_dy + brow_inner, 165, 30), style.hair, style.hair, 14U, 0U);

    // mouth
    if (emotion == 1U) {
        view.Round(Cell(area, 430, 640, 140, 26), theme::kBlood.Darkened(60), theme::kBlood, 10U, 0U);
    } else if (emotion == 2U) {
        view.Round(Cell(area, 460, 650, 80, 30), theme::kBlood.Darkened(40), theme::kBlood, 14U, 0U);
    } else if (emotion == 3U) {
        view.Round(Cell(area, 420, 635, 160, 46), theme::kBlood.Darkened(20), theme::kBlood, 20U, 0U);
    } else {
        view.Round(Cell(area, 445, 640, 110, 30), theme::kBlood.Darkened(40), theme::kBlood, 12U, 0U);
    }

    // blush
    if (emotion == 3U || style.young) {
        view.Round(Cell(area, 275, 560, 120, 46), theme::kBlood.Lightened(140), theme::kBlood.Lightened(140), 22U,
                   0U);
        view.Round(Cell(area, 605, 560, 120, 46), theme::kBlood.Lightened(140), theme::kBlood.Lightened(140), 22U,
                   0U);
    }

    if (style.beard) {
        view.Round(Cell(area, 300, 640, 400, 300), micropixel::Color::Rgb(232U, 232U, 236U),
                   micropixel::Color::Rgb(190U, 192U, 204U), 130U, 1U);
        view.Round(Cell(area, 415, 600, 170, 90), theme::kBlood.Darkened(40), theme::kBlood, 40U, 0U);
    }
}

void DrawRobe(GameView& view, const micropixel::Rect& area, micropixel::Color base, micropixel::Color trim,
              bool collar) {
    view.Round(Cell(area, 90, 780, 820, 260), base, trim, 70U, 2U);
    if (collar) {
        view.Round(Cell(area, 330, 770, 340, 90), trim, trim, 30U, 0U);
        view.Round(Cell(area, 470, 800, 60, 240), trim, trim, 24U, 0U);
    }
}

}  // namespace

uint8_t PortraitForCharacter(uint8_t character) { return character < kCharacterCount ? character : kEmptySlot; }

void Panel(GameView& view, micropixel::Rect rect, micropixel::Color fill, micropixel::Color edge, uint32_t radius) {
    view.Round(rect, fill, edge, radius, 1U);
    // A single lighter pixel row along the top edge lifts the panel off the backdrop.
    const int32_t inset = static_cast<int32_t>(radius) / 2;
    view.Fill({rect.x + inset, rect.y + 2, micropixel::math::Max(rect.width - inset * 2, 4), 1},
              edge.Lightened(70U));
}

void Header(GameContext& context, ids::Id title, bool show_gold) {
    GameView& view = context.view;
    const micropixel::Rect rect = context.layout.header;
    view.Round(rect, theme::kPanelDeep, theme::kEdge, 6U, 1U);
    view.Text({rect.x + 8, rect.y + 3}, context.strings.Get(title), theme::kAccent,
              micropixel::SystemFont::kSmall);
    if (show_gold) {
        Line purse;
        (void)purse.Append(context.strings.Get(ids::Id::kUiGold));
        (void)purse.Append(" ");
        (void)purse.AppendUint(context.progress.gold);
        view.Text({rect.x + rect.width - 46, rect.y + 3}, purse.c_str(), theme::kMuted,
                  micropixel::SystemFont::kSmall, true);
    }
}

void Portrait(GameView& view, micropixel::Rect area, uint8_t portrait, uint8_t emotion, bool highlight) {
    if (area.empty() || portrait == kEmptySlot) {
        return;
    }
    if (highlight) {
        view.Round(area.inset(-2), theme::kPanelDeep, theme::kAccent, 8U, 1U);
    } else {
        view.Round(area.inset(-2), theme::kPanelDeep, theme::kEdge, 8U, 1U);
    }

    switch (portrait) {
        case 0U: {  // 李逍遥 — young swordsman
            const FaceStyle style{micropixel::Color::Rgb(252U, 224U, 214U), micropixel::Color::Rgb(220U, 178U, 168U),
                                  micropixel::Color::Rgb(38U, 44U, 76U), micropixel::Color::Rgb(96U, 118U, 208U),
                                  micropixel::Color::Rgb(72U, 128U, 224U), 0U, false, true};
            DrawRobe(view, area, micropixel::Color::Rgb(46U, 74U, 140U), micropixel::Color::Rgb(226U, 196U, 124U),
                     true);
            DrawHead(view, area, style, emotion);
            // topknot + headband
            view.Round(Cell(area, 430, 60, 140, 130), micropixel::Color::Rgb(28U, 34U, 62U),
                       micropixel::Color::Rgb(96U, 118U, 208U), 60U, 1U);
            view.Round(Cell(area, 250, 250, 500, 60), theme::kBlood, theme::kBlood.Lightened(60U), 26U, 1U);
            break;
        }
        case 1U: {  // 赵灵儿 — water spirit
            const FaceStyle style{micropixel::Color::Rgb(255U, 232U, 236U), micropixel::Color::Rgb(228U, 190U, 198U),
                                  micropixel::Color::Rgb(214U, 216U, 236U), micropixel::Color::Rgb(255U, 255U, 255U),
                                  micropixel::Color::Rgb(92U, 196U, 196U), 1U, false, true};
            DrawRobe(view, area, micropixel::Color::Rgb(64U, 122U, 140U), micropixel::Color::Rgb(240U, 168U, 196U),
                     true);
            DrawHead(view, area, style, emotion);
            view.Round(Cell(area, 660, 300, 90, 90), theme::kBlood, theme::kBlood.Lightened(80U), 40U, 1U);
            break;
        }
        case 2U: {  // 林月如 — swordswoman of 林家堡
            const FaceStyle style{micropixel::Color::Rgb(246U, 226U, 206U), micropixel::Color::Rgb(212U, 186U, 166U),
                                  micropixel::Color::Rgb(226U, 226U, 232U), micropixel::Color::Rgb(255U, 255U, 255U),
                                  micropixel::Color::Rgb(120U, 132U, 168U), 0U, true, false};
            DrawRobe(view, area, micropixel::Color::Rgb(96U, 106U, 134U), micropixel::Color::Rgb(196U, 186U, 158U),
                     true);
            DrawHead(view, area, style, emotion);
            view.Round(Cell(area, 400, 30, 200, 120), micropixel::Color::Rgb(72U, 78U, 108U),
                       micropixel::Color::Rgb(196U, 186U, 158U), 40U, 1U);
            break;
        }
        case 3U: {  // 酒剑仙 — drunken sword immortal
            const FaceStyle style{micropixel::Color::Rgb(238U, 216U, 194U), micropixel::Color::Rgb(202U, 176U, 154U),
                                  micropixel::Color::Rgb(206U, 206U, 212U), micropixel::Color::Rgb(244U, 244U, 248U),
                                  micropixel::Color::Rgb(110U, 112U, 132U), 0U, true, false};
            DrawRobe(view, area, micropixel::Color::Rgb(96U, 74U, 62U), micropixel::Color::Rgb(178U, 150U, 110U),
                     true);
            DrawHead(view, area, style, emotion);
            break;
        }
        case 4U: {  // 村民 — villager
            const FaceStyle style{micropixel::Color::Rgb(232U, 194U, 152U), micropixel::Color::Rgb(196U, 158U, 120U),
                                  micropixel::Color::Rgb(58U, 46U, 40U), micropixel::Color::Rgb(104U, 84U, 66U),
                                  micropixel::Color::Rgb(78U, 62U, 48U), 0U, false, false};
            DrawRobe(view, area, micropixel::Color::Rgb(126U, 104U, 74U), micropixel::Color::Rgb(86U, 70U, 52U),
                     true);
            DrawHead(view, area, style, emotion);
            view.Round(Cell(area, 240U, 250U, 520U, 70U), theme::kBlood.Darkened(70U), theme::kBlood, 24U, 1U);
            break;
        }
        default: {  // 拜月教主 — moon cult leader
            const FaceStyle style{micropixel::Color::Rgb(206U, 196U, 214U), micropixel::Color::Rgb(150U, 138U, 168U),
                                  micropixel::Color::Rgb(38U, 28U, 56U), micropixel::Color::Rgb(96U, 62U, 132U),
                                  micropixel::Color::Rgb(226U, 72U, 88U), 2U, false, false};
            DrawRobe(view, area, micropixel::Color::Rgb(48U, 30U, 66U), micropixel::Color::Rgb(150U, 82U, 168U), true);
            DrawHead(view, area, style, emotion);
            break;
        }
    }
    view.Fill(Cell(area, 0, 0, 1000, 30), theme::kInk, 120U);
}

void EnemySprite(GameView& view, micropixel::Rect area, uint8_t sprite, bool alive, bool targeted) {
    if (area.empty()) {
        return;
    }
    const uint8_t alpha = alive ? 255U : 70U;
    if (targeted) {
        view.Round(area.inset(-3), micropixel::Color::Rgb(0U, 0U, 0U), theme::kAccent, 10U, 2U, 90U);
    }

    switch (sprite) {
        case 0U: {  // 蛇妖
            view.Round(Cell(area, 120, 330, 760, 480), micropixel::Color::Rgb(96U, 100U, 112U),
                       micropixel::Color::Rgb(60U, 62U, 74U), 140U, 2U, alpha);
            view.Round(Cell(area, 250, 120, 500, 420), micropixel::Color::Rgb(120U, 124U, 136U),
                       micropixel::Color::Rgb(74U, 76U, 88U), 180U, 2U, alpha);
            view.Round(Cell(area, 240, 60, 150, 220), micropixel::Color::Rgb(84U, 86U, 98U),
                       micropixel::Color::Rgb(60U, 62U, 74U), 60U, 2U, alpha);
            view.Round(Cell(area, 610, 60, 150, 220), micropixel::Color::Rgb(84U, 86U, 98U),
                       micropixel::Color::Rgb(60U, 62U, 74U), 60U, 2U, alpha);
            view.Fill(Cell(area, 330, 250, 90, 70), theme::kBlood, alpha);
            view.Fill(Cell(area, 580, 250, 90, 70), theme::kBlood, alpha);
            view.Round(Cell(area, 420, 360, 160, 120), micropixel::Color::Rgb(44U, 42U, 48U),
                       micropixel::Color::Rgb(44U, 42U, 48U), 40U, 0U, alpha);
            if (alive) {
                view.Round(Cell(area, 620, 380, 240, 90), micropixel::Color::Rgb(150U, 152U, 164U),
                           micropixel::Color::Rgb(150U, 152U, 164U), 40U, 0U, alpha);
            }
            break;
        }
        case 1U: {  // 山贼
            view.Round(Cell(area, 260, 240, 480, 620), micropixel::Color::Rgb(74U, 54U, 50U),
                       micropixel::Color::Rgb(48U, 34U, 32U), 120U, 2U, alpha);
            view.Round(Cell(area, 320, 120, 360, 340), micropixel::Color::Rgb(226U, 184U, 148U),
                       micropixel::Color::Rgb(186U, 144U, 112U), 150U, 2U, alpha);
            view.Round(Cell(area, 300, 110, 400, 150), theme::kBlood.Darkened(50U), theme::kBlood, 60U, 2U, alpha);
            view.Fill(Cell(area, 370, 300, 80, 56), theme::kInk, alpha);
            view.Fill(Cell(area, 560, 300, 80, 56), theme::kInk, alpha);
            view.Round(Cell(area, 120, 320, 130, 520), micropixel::Color::Rgb(150U, 152U, 164U),
                       micropixel::Color::Rgb(96U, 98U, 110U), 30U, 2U, alpha);
            if (!alive) {
                view.Round(Cell(area, 120, 780, 700, 140), micropixel::Color::Rgb(60U, 40U, 42U),
                           micropixel::Color::Rgb(60U, 40U, 42U), 60U, 0U, 150U);
            }
            break;
        }
        case 2U: {  // 水妖
            view.Round(Cell(area, 280, 130, 440, 520), micropixel::Color::Rgb(120U, 178U, 186U),
                       micropixel::Color::Rgb(78U, 134U, 144U), 200U, 2U, alpha);
            view.Round(Cell(area, 200, 620, 600, 320), micropixel::Color::Rgb(96U, 152U, 164U),
                       micropixel::Color::Rgb(78U, 134U, 144U), 150U, 0U, static_cast<uint8_t>(alpha * 3U / 4U));
            view.Fill(Cell(area, 380, 320, 90, 90), micropixel::Color::Rgb(24U, 40U, 44U), alpha);
            view.Fill(Cell(area, 540, 320, 90, 90), micropixel::Color::Rgb(24U, 40U, 44U), alpha);
            view.Round(Cell(area, 430, 500, 140, 60), micropixel::Color::Rgb(24U, 40U, 44U),
                       micropixel::Color::Rgb(24U, 40U, 44U), 28U, 0U, alpha);
            break;
        }
        default: {  // 拜月教主
            view.Round(Cell(area, 180, 520, 640, 440), micropixel::Color::Rgb(40U, 24U, 58U),
                       micropixel::Color::Rgb(120U, 62U, 150U), 120U, 2U, alpha);
            view.Round(Cell(area, 300, 180, 400, 400), micropixel::Color::Rgb(214U, 202U, 220U),
                       micropixel::Color::Rgb(150U, 138U, 168U), 160U, 2U, alpha);
            view.Round(Cell(area, 220, 40, 150, 260), theme::kBlood.Darkened(50U), theme::kBlood, 60U, 2U, alpha);
            view.Round(Cell(area, 630, 40, 150, 260), theme::kBlood.Darkened(50U), theme::kBlood, 60U, 2U, alpha);
            view.Round(Cell(area, 340, 300, 120, 70), theme::kBlood, theme::kBlood, 20U, 0U, alpha);
            view.Round(Cell(area, 540, 300, 120, 70), theme::kBlood, theme::kBlood, 20U, 0U, alpha);
            view.Round(Cell(area, 400, 460, 200, 50), micropixel::Color::Rgb(30U, 18U, 40U),
                       micropixel::Color::Rgb(30U, 18U, 40U), 18U, 0U, alpha);
            break;
        }
    }
}

void HeroToken(GameView& view, micropixel::Rect area, uint8_t character, uint8_t facing) {
    micropixel::Color robe = micropixel::Color::Rgb(70U, 108U, 196U);
    micropixel::Color hair = micropixel::Color::Rgb(32U, 36U, 62U);
    micropixel::Color skin = micropixel::Color::Rgb(250U, 224U, 212U);
    if (character == kCharLingxi) {
        robe = micropixel::Color::Rgb(72U, 150U, 158U);
        hair = micropixel::Color::Rgb(232U, 234U, 246U);
    } else if (character == kCharYunyang) {
        robe = micropixel::Color::Rgb(122U, 126U, 150U);
        hair = micropixel::Color::Rgb(232U, 232U, 236U);
    }

    const int32_t w = area.width;
    const int32_t h = area.height;
    const int32_t cx = area.x + w / 2;
    // shadow
    view.Round({area.x + w / 6, area.y + h - h / 8, w * 2 / 3, h / 10}, micropixel::Color::Black(),
               micropixel::Color::Black(), h / 12, 0U, 90U);
    // body
    view.Round({area.x + w / 5, area.y + h / 2, w * 3 / 5, h * 2 / 5}, robe, robe.Darkened(50U), w / 8, 1U);
    // head
    const int32_t head = w * 3 / 5;
    view.Round({cx - head / 2, area.y + h / 6, head, head}, skin, skin.Darkened(40U), head / 2, 1U);
    // hair
    view.Round({cx - head / 2 - w / 20, area.y + h / 8, head + w / 10, head / 2}, hair, hair.Lightened(50U),
               head / 3, 1U);
    // facing marker
    const micropixel::Rect marker = facing == kFaceUp     ? micropixel::Rect{area.x + w / 3, area.y + h / 12, w / 3, h / 16}
                                    : facing == kFaceLeft ? micropixel::Rect{area.x + w / 12, area.y + h / 3, w / 6, h / 10}
                                    : facing == kFaceRight
                                        ? micropixel::Rect{area.x + w * 3 / 4, area.y + h / 3, w / 6, h / 10}
                                        : micropixel::Rect{area.x + w / 3, area.y + h * 3 / 4, w / 3, h / 12};
    view.Fill(marker, theme::kAccent, 220U);
}

void DrawTile(GameView& view, micropixel::Rect area, uint8_t tile) {
    if (area.empty()) {
        return;
    }
    switch (static_cast<ashes::Tile>(tile)) {
        case ashes::Tile::kGrass:
            view.Fill(area, theme::kGrass);
            view.Fill({area.x + area.width / 5, area.y + area.height / 3, area.width / 6, area.height / 6},
                      theme::kGrassAlt);
            break;
        case Tile::kPath:
            view.Fill(area, theme::kPath);
            view.Fill({area.x, area.y + area.height / 2, area.width, 1}, theme::kPath.Darkened(30U));
            break;
        case Tile::kTree:
            view.Fill(area, theme::kGrass.Darkened(30U));
            view.Round({area.x + area.width / 8, area.y + area.height / 8, area.width * 3 / 4, area.height * 3 / 4},
                       theme::kTree, theme::kTreeTop, area.width / 3, 1U);
            break;
        case Tile::kWater:
            view.Fill(area, theme::kWater);
            view.Fill({area.x + area.width / 6, area.y + area.height / 3, area.width * 2 / 3, 1},
                      theme::kWater.Lightened(70U));
            break;
        case Tile::kRoof:
            view.Fill(area, theme::kRoof);
            view.Fill({area.x, area.y, area.width, area.height / 6}, theme::kRoofTop);
            view.Fill({area.x, area.y + area.height * 2 / 3, area.width, 1}, theme::kRoof.Darkened(50U));
            break;
        case Tile::kWall:
            view.Fill(area, theme::kWall);
            view.Fill({area.x, area.y, area.width, area.height / 5}, theme::kWallTop);
            break;
        case Tile::kFloor:
            view.Fill(area, theme::kFloor);
            view.Fill({area.x, area.y, area.width, 1}, theme::kFloor.Darkened(40U));
            view.Fill({area.x, area.y, 1, area.height}, theme::kFloor.Darkened(40U));
            break;
        case Tile::kShrine:
            view.Fill(area, theme::kFloor.Darkened(30U));
            view.Round({area.x + area.width / 6, area.y + area.height / 6, area.width * 2 / 3, area.height * 2 / 3},
                       theme::kShrine, theme::kShrine.Lightened(90U), area.width / 4, 1U);
            break;
    }
}

void NameTag(GameView& view, micropixel::Rect area, const char* text, micropixel::Color color) {
    if (area.empty()) {
        return;
    }
    view.Round(area, micropixel::Color::Rgb(10U, 12U, 24U), color, 5U, 1U, 200U);
    view.CenterText(area.center_x(), area.y + 2, text, color, micropixel::SystemFont::kSmall);
}

void HpMpBars(GameView& view, micropixel::Rect area, const Combatant& unit, bool show_mp) {
    const int32_t bar_h = micropixel::math::Max(area.height / 4, 5);
    view.Bar({area.x, area.y, area.width, bar_h}, unit.hp, unit.max_hp, theme::kHp);
    if (show_mp) {
        view.Bar({area.x, area.y + bar_h + 2, area.width, bar_h}, unit.mp, unit.max_mp, theme::kMp);
    }
}

}  // namespace widgets
}  // namespace ashes
