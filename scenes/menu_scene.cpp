// SPDX-License-Identifier: Apache-2.0
// Status menu: party sheets, the bag and the save / resume actions.

#include "../pal_common.hpp"
#include "../pal_widgets.hpp"

#include "sdk/math.hpp"

namespace pal {
namespace {

inline constexpr uint8_t kMenuSave = 0U;
inline constexpr uint8_t kMenuResume = 1U;
inline constexpr uint8_t kMenuButtons = 2U;

micropixel::Rect CardRect(const GameContext& context, uint8_t index, uint8_t count) {
    const micropixel::Rect body = context.layout.body;
    const int32_t area = body.height * 58 / 100;
    const int32_t pad = 5;
    const int32_t rows = count == 0U ? 1 : count;
    const int32_t height = micropixel::math::Max((area - pad * (rows + 1)) / rows, 28);
    return {body.x + pad, body.y + pad + static_cast<int32_t>(index) * (height + pad), body.width - pad * 2, height};
}

micropixel::Rect BagRect(const GameContext& context, uint8_t index, uint8_t count) {
    const micropixel::Rect body = context.layout.body;
    const int32_t top = body.y + body.height * 58 / 100;
    const int32_t pad = 5;
    const int32_t rows = micropixel::math::Max<int32_t>(count, 1);
    const int32_t height = micropixel::math::Max((body.y + body.height - top - pad * 2) / rows, 13);
    return {body.x + pad, top + pad + static_cast<int32_t>(index) * height, body.width - pad * 2, height - 1};
}

micropixel::Rect ButtonRect(const GameContext& context, uint8_t index) {
    const micropixel::Rect footer = context.layout.footer;
    const int32_t pad = 8;
    const int32_t width = (footer.width - pad * 3) / 2;
    const int32_t height = micropixel::math::Clamp<int32_t>(footer.height / 3, 28, 52);
    return {footer.x + pad + static_cast<int32_t>(index) * (width + pad), footer.y + (footer.height - height) / 2,
            width, height};
}

void Activate(GameContext& context, uint8_t index) {
    if (index == kMenuSave) {
        StoreProgress(context);
        context.notice_text = ids::Id::kUiSaved;
        context.notice_ms = 1500U;
        context.dirty = true;
        return;
    }
    PushScene(context, context.previous_scene);
}

// Index of the party member who most needs `effect`, or -1 when nobody does.
int32_t FieldTarget(const Progress& progress, ItemEffect effect) {
    int32_t best = -1;
    uint32_t best_ratio = 1000U;
    for (uint8_t index = 0U; index < progress.party_size && index < kMaxParty; ++index) {
        const PartyMember& member = progress.party[index];
        if (member.character == kEmptySlot) {
            continue;
        }
        switch (effect) {
            case ItemEffect::kHealHp: {
                const uint32_t max_hp = MemberMaxHp(member);
                const uint32_t ratio = max_hp == 0U ? 1000U : member.hp * 1000U / max_hp;
                if (member.hp > 0U && member.hp < max_hp && ratio < best_ratio) {
                    best_ratio = ratio;
                    best = index;
                }
                break;
            }
            case ItemEffect::kHealMp: {
                const uint32_t max_mp = MemberMaxMp(member);
                const uint32_t ratio = max_mp == 0U ? 1000U : member.mp * 1000U / max_mp;
                if (member.hp > 0U && member.mp < max_mp && ratio < best_ratio) {
                    best_ratio = ratio;
                    best = index;
                }
                break;
            }
            case ItemEffect::kCurePoison:
                if (member.poisoned && best < 0) {
                    best = index;
                }
                break;
            case ItemEffect::kRevive:
                if (member.hp == 0U && best < 0) {
                    best = index;
                }
                break;
            default:
                break;
        }
    }
    return best;
}

// Uses one bag item outside battle on whoever needs it most.
void UseBagItem(GameContext& context, uint8_t slot) {
    Progress& progress = context.progress;
    context.notice_text = ids::Id::kUiNoEffect;
    context.notice_ms = 1200U;
    if (slot >= progress.bag_size) {
        return;
    }
    const uint8_t item_id = progress.bag[slot].item;
    const ItemDef& item = Item(item_id);
    if (!item.field_usable) {
        return;
    }
    const int32_t target = FieldTarget(progress, item.effect);
    if (target < 0) {
        return;
    }
    PartyMember& member = progress.party[target];
    switch (item.effect) {
        case ItemEffect::kHealHp:
            member.hp = static_cast<uint16_t>(micropixel::math::Min<uint32_t>(MemberMaxHp(member), member.hp + item.magnitude));
            break;
        case ItemEffect::kHealMp:
            member.mp = static_cast<uint16_t>(micropixel::math::Min<uint32_t>(MemberMaxMp(member), member.mp + item.magnitude));
            break;
        case ItemEffect::kCurePoison:
            member.poisoned = false;
            break;
        case ItemEffect::kRevive:
            member.hp = static_cast<uint16_t>(micropixel::math::Min<uint32_t>(MemberMaxHp(member), item.magnitude));
            break;
        default:
            return;
    }
    (void)BagConsume(progress, item_id, 1U);
    context.notice_text = ids::Id::kUiUsed;
}

}  // namespace

void MenuSceneEnter(GameContext& context) {
    context.cursor = kMenuResume;
    context.notice_ms = 0U;
    context.dirty = true;
}

void MenuSceneUpdate(GameContext& context, uint32_t delta_ms) {
    if (context.notice_ms > 0U) {
        context.notice_ms = context.notice_ms > delta_ms ? context.notice_ms - delta_ms : 0U;
        context.dirty = true;  // the notice pill just moved or disappeared
    }
}

bool MenuSceneTouch(GameContext& context, const micropixel::TouchEvent& touch) {
    if (touch.phase() != micropixel::TouchPhase::kDown) {
        return false;
    }
    for (uint8_t index = 0U; index < kMenuButtons; ++index) {
        if (ButtonRect(context, index).contains(touch.position())) {
            context.cursor = index;
            Activate(context, index);
            return true;
        }
    }
    // Tapping a bag row uses that item on whoever needs it.
    const uint8_t bag_rows = micropixel::math::Max<uint8_t>(context.progress.bag_size, 1U);
    for (uint8_t index = 0U; index < context.progress.bag_size; ++index) {
        if (BagRect(context, index, bag_rows).contains(touch.position())) {
            UseBagItem(context, index);
            context.dirty = true;
            return true;
        }
    }
    return true;
}

bool MenuSceneKey(GameContext& context, micropixel::KeyCode code) {
    switch (code) {
        case micropixel::KeyCode::kLeft:
        case micropixel::KeyCode::kRight:
        case micropixel::KeyCode::kUp:
        case micropixel::KeyCode::kDown:
            context.cursor = context.cursor == 0U ? 1U : 0U;
            context.dirty = true;
            return true;
        case micropixel::KeyCode::kConfirm:
        case micropixel::KeyCode::kSouth:
            Activate(context, context.cursor);
            return true;
        case micropixel::KeyCode::kBack:
        case micropixel::KeyCode::kMenu:
            PushScene(context, context.previous_scene);
            return true;
        default:
            return false;
    }
}

void MenuSceneRender(GameContext& context) {
    GameView& view = context.view;
    const GameLayout& layout = context.layout;
    view.Begin();
    view.Fill(layout.screen, theme::kInk);
    widgets::Header(context, ids::Id::kUiStatus, true);

    // Party sheets.
    const uint8_t members = micropixel::math::Min<uint8_t>(context.progress.party_size, kMaxParty);
    for (uint8_t index = 0U; index < members; ++index) {
        const PartyMember& member = context.progress.party[index];
        const micropixel::Rect card = CardRect(context, index, members);
        widgets::Panel(view, card, theme::kPanel, theme::kEdge, 8U);

        const int32_t face = card.height - 8;
        widgets::Portrait(view, {card.x + 4, card.y + 4, face, face},
                          widgets::PortraitForCharacter(member.character), member.hp == 0U ? 2U : 0U, false);

        const int32_t text_x = card.x + face + 10;
        view.Text({text_x, card.y + 3}, context.strings.Get(MemberNameId(member)),
                  member.hp > 0U ? theme::kText : theme::kDim, micropixel::SystemFont::kSmall);

        Line level;
        (void)level.Append(context.strings.Get(ids::Id::kUiLv));
        (void)level.AppendUint(member.level);
        view.Text({card.x + card.width - 6, card.y + 3}, level.c_str(), theme::kAccent,
                  micropixel::SystemFont::kSmall, true);

        const micropixel::Rect bars{text_x, card.y + card.height / 2 - 2, card.width - face - 20,
                                    micropixel::math::Max(card.height / 4, 12)};
        view.Bar({bars.x, bars.y, bars.width, 6}, member.hp, MemberMaxHp(member), theme::kHp);
        view.Bar({bars.x, bars.y + 8, bars.width, 6}, member.mp, MemberMaxMp(member), theme::kMp);

        Line stats;
        (void)stats.AppendUint(member.hp);
        (void)stats.Append("/");
        (void)stats.AppendUint(MemberMaxHp(member));
        (void)stats.Append("  ");
        (void)stats.AppendUint(member.mp);
        (void)stats.Append("/");
        (void)stats.AppendUint(MemberMaxMp(member));
        view.Text({text_x, card.y + card.height - 14}, stats.c_str(), theme::kMuted,
                  micropixel::SystemFont::kSmall);

        Line combat;
        (void)combat.Append(context.strings.Get(ids::Id::kUiAtk));
        (void)combat.AppendUint(MemberAttack(member));
        (void)combat.Append(" ");
        (void)combat.Append(context.strings.Get(ids::Id::kUiDef));
        (void)combat.AppendUint(MemberDefense(member));
        (void)combat.Append(" ");
        (void)combat.Append(context.strings.Get(ids::Id::kUiSpd));
        (void)combat.AppendUint(MemberSpeed(member));
        view.Text({card.x + card.width - 6, card.y + card.height - 14}, combat.c_str(), theme::kMuted,
                  micropixel::SystemFont::kSmall, true);
    }

    // Bag.
    const uint8_t bag_rows = micropixel::math::Max<uint8_t>(context.progress.bag_size, 1U);
    for (uint8_t index = 0U; index < bag_rows; ++index) {
        const micropixel::Rect row = BagRect(context, index, bag_rows);
        if (index >= context.progress.bag_size) {
            view.Text({row.x + 6, row.y + 1}, context.strings.Get(ids::Id::kUiEmpty), theme::kDim,
                      micropixel::SystemFont::kSmall);
            continue;
        }
        const BagSlot& slot = context.progress.bag[index];
        view.Text({row.x + 6, row.y + 1}, context.strings.Get(Item(slot.item).name), theme::kText,
                  micropixel::SystemFont::kSmall);
        Line amount;
        (void)amount.Append("x");
        (void)amount.AppendUint(slot.count);
        view.Text({row.x + row.width - 6, row.y + 1}, amount.c_str(), theme::kMuted,
                  micropixel::SystemFont::kSmall, true);
    }

    // Actions.
    for (uint8_t index = 0U; index < kMenuButtons; ++index) {
        const micropixel::Rect rect = ButtonRect(context, index);
        const bool focused = context.cursor == index;
        view.Round(rect, focused ? theme::kPanel : theme::kPanelDeep,
                   focused ? theme::kAccent : theme::kEdge, 10U, focused ? 2U : 1U);
        view.CenterText(rect.center_x(), rect.center_y() - 7,
                        index == kMenuSave ? context.strings.Get(ids::Id::kUiSave)
                                           : context.strings.Get(ids::Id::kUiResume),
                        focused ? theme::kHighlight : theme::kText, micropixel::SystemFont::kSmall);
    }

    if (context.notice_ms > 0U) {
        const micropixel::Rect notice{layout.footer.x, layout.footer.y - 22, layout.footer.width, 18};
        widgets::NameTag(view, {layout.footer.center_x() - 60, notice.y, 120, 17},
                         context.strings.Get(context.notice_text), theme::kJade);
    }

    view.End();
}

}  // namespace pal
