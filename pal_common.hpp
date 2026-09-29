// SPDX-License-Identifier: Apache-2.0
// Shared foundation for 仙剑奇侠传: the palette, the responsive layout, the
// immediate-mode scene view and the GameContext every scene receives.
//
// The structure mirrors the official guest/apps/sdk-demo: this header is the
// app's `demo_page.hpp`, each scenes/*.cpp is one `pages/*.cpp`.

#ifndef PAL_COMMON_HPP
#define PAL_COMMON_HPP

#include <stdint.h>

#include <array>
#include <vector>

#include "pal_strings.hpp"
#include "sdk/micropixel.hpp"

#include "pal_battle.hpp"
#include "pal_content.hpp"
#include "pal_dialogue.hpp"
#include "pal_model.hpp"
#include "pal_world.hpp"

namespace pal {

using Line = micropixel::FixedString<160U>;
using ShortLine = micropixel::FixedString<48U>;

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------

namespace theme {

inline constexpr micropixel::Color kInk = micropixel::Color::Rgb(9U, 11U, 24U);
inline constexpr micropixel::Color kNight = micropixel::Color::Rgb(15U, 19U, 38U);
inline constexpr micropixel::Color kPanel = micropixel::Color::Rgb(23U, 28U, 54U);
inline constexpr micropixel::Color kPanelDeep = micropixel::Color::Rgb(14U, 18U, 38U);
inline constexpr micropixel::Color kEdge = micropixel::Color::Rgb(70U, 88U, 150U);
inline constexpr micropixel::Color kAccent = micropixel::Color::Rgb(228U, 196U, 124U);
inline constexpr micropixel::Color kJade = micropixel::Color::Rgb(118U, 214U, 186U);
inline constexpr micropixel::Color kBlood = micropixel::Color::Rgb(210U, 90U, 106U);
inline constexpr micropixel::Color kText = micropixel::Color::Rgb(234U, 235U, 248U);
inline constexpr micropixel::Color kMuted = micropixel::Color::Rgb(148U, 156U, 190U);
inline constexpr micropixel::Color kDim = micropixel::Color::Rgb(96U, 104U, 134U);
inline constexpr micropixel::Color kHighlight = micropixel::Color::Rgb(255U, 232U, 178U);

inline constexpr micropixel::Color kHp = micropixel::Color::Rgb(226U, 110U, 124U);
inline constexpr micropixel::Color kMp = micropixel::Color::Rgb(108U, 166U, 236U);
inline constexpr micropixel::Color kBarBack = micropixel::Color::Rgb(34U, 40U, 66U);

// exploration tiles
inline constexpr micropixel::Color kGrass = micropixel::Color::Rgb(56U, 84U, 62U);
inline constexpr micropixel::Color kGrassAlt = micropixel::Color::Rgb(64U, 96U, 70U);
inline constexpr micropixel::Color kPath = micropixel::Color::Rgb(124U, 108U, 82U);
inline constexpr micropixel::Color kTree = micropixel::Color::Rgb(34U, 58U, 46U);
inline constexpr micropixel::Color kTreeTop = micropixel::Color::Rgb(48U, 80U, 58U);
inline constexpr micropixel::Color kWater = micropixel::Color::Rgb(44U, 84U, 132U);
inline constexpr micropixel::Color kRoof = micropixel::Color::Rgb(112U, 74U, 66U);
inline constexpr micropixel::Color kRoofTop = micropixel::Color::Rgb(140U, 96U, 84U);
inline constexpr micropixel::Color kWall = micropixel::Color::Rgb(80U, 84U, 104U);
inline constexpr micropixel::Color kWallTop = micropixel::Color::Rgb(104U, 108U, 130U);
inline constexpr micropixel::Color kFloor = micropixel::Color::Rgb(118U, 114U, 108U);
inline constexpr micropixel::Color kShrine = micropixel::Color::Rgb(196U, 170U, 96U);

}  // namespace theme

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

struct GameLayout final {
    micropixel::Rect screen{};
    micropixel::Rect header{};
    micropixel::Rect body{};
    micropixel::Rect footer{};
    micropixel::Rect stage{};       // exploration viewport
    micropixel::Rect message{};     // battle message strip
    micropixel::Rect command{};     // battle command panel
    micropixel::Rect dialogue{};    // dialogue box
    micropixel::Rect portrait{};    // speaker portrait inside the dialogue box
    micropixel::Rect dialogue_text{};
    micropixel::Rect choices{};     // choice list inside the dialogue box
    micropixel::Rect enemy_area{};
    micropixel::Rect party_area{};
    micropixel::Rect menu_button{};
    micropixel::Rect list{};  // scrollable content for the menu scene
    std::array<micropixel::Rect, 4U> dpad{};
    std::array<micropixel::Rect, 6U> commands{};
    bool compact{};
};

GameLayout BuildLayout(const micropixel::RendererInfo& info);

// ---------------------------------------------------------------------------
// Immediate-mode scene view
// ---------------------------------------------------------------------------

// Pools retained nodes and recycles them every frame, exactly like the official
// DemoView. Scenes describe the frame declaratively; nothing is retained
// between frames, so screens never leak node bookkeeping.
//
// Every rectangle goes through RoundedRectNode — a plain fill is just a
// rounded rect with radius 0 — so a layer has exactly one rect pool and its
// pool index order is the draw-call order. Mixing ShapeNode and
// RoundedRectNode in one layer breaks that: both are recycled by their own
// index, so a rounded panel can end up composited above a fill that was drawn
// after it (buttons swallow their arrows, panels swallow their bars, portraits
// swallow their eye highlights). Labels live in their own container above the
// rects, which matches every screen here.
//
// A second, clipped "map" layer exists for the exploration viewport: content
// drawn through UseMapLayer(true) can never spill over the HUD.
class GameView final {
   public:
    GameView(micropixel::Renderer renderer, micropixel::Log log, micropixel::Scene& scene,
             micropixel::ContainerNode root)
        : renderer_(renderer), log_(log), scene_(scene), map_(root, 0, 1), hud_(root, 2, 3) {}

    // `map_clip` bounds the map layer; pass {} for screens without one.
    void Begin(micropixel::Rect map_clip = {}) {
        updating_ = true;
        // Both containers of the map layer are clipped: a rounded rect and a
        // label are separate nodes, so clipping only the rects would still let a
        // floating name tag escape over the HUD.
        map_.rects.SetClip(map_clip);
        map_.labels.SetClip(map_clip);
        map_.Hide();
        hud_.Hide();
    }

    // Routes subsequent Fill/Round/Text into the clipped map layer.
    void UseMapLayer(bool enabled) { on_map_layer_ = enabled; }

    void End() {
        const uint32_t rects = map_.rect_count + hud_.rect_count;
        const uint32_t labels = map_.label_count + hud_.label_count;
        map_.Retire();
        hud_.Retire();
        updating_ = false;
        const auto presented = renderer_.Present(scene_);
        if (!presented.has_value()) {
            // A rejected frame leaves the previous one on screen. Report it with
            // the frame's node counts and keep running: trapping here would take
            // the whole App down, and the next frame can still recover.
            Line line;
            (void)line.Append("pal: present rejected: ");
            (void)line.Append(presented.error().name());
            (void)line.Append(" rects=");
            (void)line.AppendUint(rects);
            (void)line.Append(" labels=");
            (void)line.AppendUint(labels);
            (void)line.Append(" nodes=");
            (void)line.AppendUint(scene_.node_count());
            (void)line.Append(" used=");
            (void)line.AppendUint(rects + labels);
            log_.Error(line.c_str());
        }
    }

    // Plain filled rectangle.
    void Fill(micropixel::Rect rect, micropixel::Color color, uint8_t opacity = 255U) {
        Rect(rect, color, color, 0U, 0U, opacity);
    }

    // Rounded rectangle with an optional border.
    void Round(micropixel::Rect rect, micropixel::Color fill, micropixel::Color stroke, uint32_t radius,
               uint32_t stroke_width = 1U, uint8_t opacity = 255U) {
        Rect(rect, fill, stroke, radius, stroke_width, opacity);
    }

    void Text(micropixel::Point position, const char* text, micropixel::Color color,
              micropixel::SystemFont font = micropixel::SystemFont::kMedium, bool centered = false) {
        micropixel::Assert(updating_, "pal: no active view frame");
        // The Host rejects a zero-length label outright (scene_graph.cpp
        // TextLength() panics), so an empty string — a typewriter line that has
        // not revealed its first codepoint yet — simply draws nothing.
        if (text == nullptr || text[0] == '\0') {
            return;
        }
        Layer& layer = Current();
        if (layer.label_count == layer.label_nodes.size()) {
            layer.label_nodes.push_back(
                layer.labels.CreateLabel(position, text, color, font, centered).value());
        }
        micropixel::LabelNode& node = layer.label_nodes[layer.label_count++];
        node.SetPosition(position);
        node.SetText(text);
        node.SetColor(color);
        node.SetFont(font);
        node.SetCentered(centered);
        node.SetVisible(true);
    }

    void CenterText(int32_t center_x, int32_t y, const char* text, micropixel::Color color,
                    micropixel::SystemFont font = micropixel::SystemFont::kMedium) {
        Text({center_x, y}, text, color, font, true);
    }

    // Two-tone bar with a recessed trough; `value` beyond `maximum` is clamped.
    void Bar(micropixel::Rect rect, uint32_t value, uint32_t maximum, micropixel::Color fill) {
        Fill(rect, theme::kBarBack);
        if (maximum == 0U || value == 0U || rect.empty()) {
            return;
        }
        const uint32_t clamped = value > maximum ? maximum : value;
        int32_t filled = static_cast<int32_t>(static_cast<uint32_t>(rect.width) * clamped / maximum);
        if (filled < 1) {
            filled = 1;
        }
        Fill({rect.x, rect.y, filled, rect.height}, fill);
    }

   private:
    // One retained layer: a rect container, a label container above it, and the
    // recycled node pools.
    struct Layer final {
        Layer(micropixel::ContainerNode root, int16_t rect_z, int16_t label_z) {
            rects = root.CreateContainer({.z_order = rect_z}).value();
            labels = root.CreateContainer({.z_order = label_z}).value();
        }

        void Hide() {
            for (micropixel::RoundedRectNode& node : rect_nodes) {
                node.SetVisible(false);
            }
            for (micropixel::LabelNode& node : label_nodes) {
                node.SetVisible(false);
            }
        }

        void Retire() {
            for (uint32_t index = rect_count; index < rect_nodes.size(); ++index) {
                rect_nodes[index].Destroy();
            }
            for (uint32_t index = label_count; index < label_nodes.size(); ++index) {
                label_nodes[index].Destroy();
            }
            rect_nodes.resize(rect_count);
            label_nodes.resize(label_count);
            rect_count = 0U;
            label_count = 0U;
        }

        micropixel::ContainerNode rects{};
        micropixel::ContainerNode labels{};
        std::vector<micropixel::RoundedRectNode> rect_nodes{};
        std::vector<micropixel::LabelNode> label_nodes{};
        uint32_t rect_count{};
        uint32_t label_count{};
    };

    Layer& Current() { return on_map_layer_ ? map_ : hud_; }

    void Rect(micropixel::Rect rect, micropixel::Color fill, micropixel::Color stroke, uint32_t radius,
              uint32_t stroke_width, uint8_t opacity) {
        micropixel::Assert(updating_, "pal: no active view frame");
        if (rect.empty() || opacity == 0U) {
            return;
        }
        // A corner radius can never exceed half the shorter side, and neither can
        // the border: the Host rejects the entire frame (kInternal) otherwise.
        // The vector art below asks for generously rounded shapes ("as round as
        // possible") at many sizes, so clamp here once instead of at every call
        // site. A radius of exactly half side is a pill/circle, which is what the
        // art intends.
        const int32_t shorter = micropixel::math::Min(rect.width, rect.height);
        const uint32_t limit = static_cast<uint32_t>(shorter > 0 ? shorter : 0) / 2U;
        if (radius > limit) {
            radius = limit;
        }
        if (stroke_width > limit) {
            stroke_width = limit;
        }
        Layer& layer = Current();
        if (layer.rect_count == layer.rect_nodes.size()) {
            micropixel::RoundedRectStyle style{};
            style.fill = fill;
            style.stroke = stroke;
            style.radius = radius;
            style.stroke_width = stroke_width;
            layer.rect_nodes.push_back(layer.rects.CreateRoundedRect(rect, style).value());
        }
        micropixel::RoundedRectNode& node = layer.rect_nodes[layer.rect_count++];
        node.SetRect(rect);
        node.SetFillColor(fill);
        node.SetStrokeColor(stroke);
        node.SetRadius(radius);
        node.SetStrokeWidth(stroke_width);
        node.SetOpacity(opacity);
        node.SetVisible(true);
    }

    micropixel::Renderer renderer_;
    micropixel::Log log_;
    micropixel::Scene& scene_;
    Layer map_;
    Layer hud_;
    bool on_map_layer_{};
    bool updating_{};
};

// ---------------------------------------------------------------------------
// Scenes
// ---------------------------------------------------------------------------

enum SceneId : uint8_t {
    kSceneTitle = 0U,
    kSceneExplore,
    kSceneDialogue,
    kSceneBattle,
    kSceneMenu,
    kSceneEnding,
    kSceneCount,
};

// What the dialogue scene should do once the last line is dismissed.
enum DialogueReturn : uint8_t {
    kDialogueToExplore = 0U,  // an NPC chat over the map
    kDialogueToScript,        // a chapter beat: advance the script
    kDialogueToEnding,        // the epilogue: roll the ending screen
};

inline constexpr uint8_t kNoDialogue = 0xFFU;

struct GameContext final {
    GameContext(micropixel::Application& application, ids::Catalog catalog, micropixel::InputInfo input_info,
                GameLayout layout_value, micropixel::Scene& scene_ref, micropixel::ContainerNode root_node,
                GameView& view_ref, micropixel::XorShift32 generator)
        : app(application),
          strings(catalog),
          input(input_info),
          layout(layout_value),
          scene(scene_ref),
          root(root_node),
          view(view_ref),
          rng(generator) {}

    micropixel::Application& app;
    ids::Catalog strings;
    micropixel::InputInfo input;
    GameLayout layout;
    micropixel::Scene& scene;
    micropixel::ContainerNode root;
    GameView& view;

    Progress progress{};
    WorldState world{};
    DialogueState dialogue{};
    BattleState battle{};
    micropixel::XorShift32 rng{};

    uint8_t scene_id{kSceneTitle};
    uint8_t previous_scene{kSceneTitle};  // the menu returns here
    uint8_t cursor{};                     // shared menu cursor

    // Drag-to-walk state for the exploration map: where the thumb last was,
    // where the gesture started, and whether a drag is in flight.
    int32_t drag_x{};
    int32_t drag_y{};
    int32_t drag_origin_x{};
    int32_t drag_origin_y{};
    bool dragging{};
    uint8_t dialogue_return{kDialogueToScript};
    uint32_t notice_ms{};  // transient confirmation ("已记录天机") countdown
    bool has_save{};
    bool dirty{true};       // re-render when set
    bool refresh_layout{};  // display metrics changed (resume / rotation)
};

// Selected scene lifecycle. Every scene implements the same five entry points.
void TitleSceneEnter(GameContext& context);
void TitleSceneUpdate(GameContext& context, uint32_t delta_ms);
bool TitleSceneTouch(GameContext& context, const micropixel::TouchEvent& touch);
bool TitleSceneKey(GameContext& context, micropixel::KeyCode code);
void TitleSceneRender(GameContext& context);

void ExploreSceneEnter(GameContext& context);
void ExploreSceneUpdate(GameContext& context, uint32_t delta_ms);
bool ExploreSceneTouch(GameContext& context, const micropixel::TouchEvent& touch);
bool ExploreSceneKey(GameContext& context, micropixel::KeyCode code);
void ExploreSceneRender(GameContext& context);

void DialogueSceneEnter(GameContext& context);
void DialogueSceneUpdate(GameContext& context, uint32_t delta_ms);
bool DialogueSceneTouch(GameContext& context, const micropixel::TouchEvent& touch);
bool DialogueSceneKey(GameContext& context, micropixel::KeyCode code);
void DialogueSceneRender(GameContext& context);

void BattleSceneEnter(GameContext& context);
void BattleSceneUpdate(GameContext& context, uint32_t delta_ms);
bool BattleSceneTouch(GameContext& context, const micropixel::TouchEvent& touch);
bool BattleSceneKey(GameContext& context, micropixel::KeyCode code);
void BattleSceneRender(GameContext& context);

void MenuSceneEnter(GameContext& context);
void MenuSceneUpdate(GameContext& context, uint32_t delta_ms);
bool MenuSceneTouch(GameContext& context, const micropixel::TouchEvent& touch);
bool MenuSceneKey(GameContext& context, micropixel::KeyCode code);
void MenuSceneRender(GameContext& context);

void EndingSceneEnter(GameContext& context);
void EndingSceneUpdate(GameContext& context, uint32_t delta_ms);
bool EndingSceneTouch(GameContext& context, const micropixel::TouchEvent& touch);
bool EndingSceneKey(GameContext& context, micropixel::KeyCode code);
void EndingSceneRender(GameContext& context);

// ---------------------------------------------------------------------------
// Scene switching helpers shared by the screens (implemented in pal_app.cpp)
// ---------------------------------------------------------------------------

// Creates the dialogue state for `node_id` and switches to the dialogue scene.
void PushDialogue(GameContext& context, uint8_t node_id, uint8_t return_kind);
// Starts `battle_id` and switches to the battle scene.
void PushBattle(GameContext& context, uint8_t battle_id);
// Switches to an arbitrary scene id.
void PushScene(GameContext& context, uint8_t scene_id);
// Runs the chapter script step at progress.script_index.
void EnterScriptStep(GameContext& context);
// Moves to the next script step (or the ending) and stores the save.
void AdvanceScript(GameContext& context);
// Writes the save blob; returns false when the Host rejects it.
bool StoreProgress(GameContext& context);

// Current scene's party leader name, for the header bar.
ids::Id ContextMapNameId(const GameContext& context);

// One structured line to the Host log. Used to trace the chapter flow on a
// real device, where stepping through a debugger is not an option.
void LogTrace(GameContext& context, const char* label, int32_t first, int32_t second);

}  // namespace pal

#endif  // PAL_COMMON_HPP
