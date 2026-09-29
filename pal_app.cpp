// SPDX-License-Identifier: Apache-2.0
// Application glue: the retained scene, the event loop, the chapter script
// machine and the scene routing. This is the app's `demo_app.cpp`.

#include "pal.hpp"

#include "pal_common.hpp"
#include "pal_widgets.hpp"

#include "sdk/math.hpp"

namespace pal {

void LogTrace(GameContext& context, const char* label, int32_t first, int32_t second) {
    Line line;
    (void)line.Append("pal: ");
    (void)line.Append(label);
    (void)line.Append(" ");
    (void)line.AppendInt(first);
    (void)line.Append(" ");
    (void)line.AppendInt(second);
    context.app.log().Info(line.c_str());
}

// ---------------------------------------------------------------------------
// Scene routing and the chapter script
// ---------------------------------------------------------------------------

void PushScene(GameContext& context, uint8_t scene_id) {
    context.scene_id = scene_id < kSceneCount ? scene_id : kSceneTitle;
    LogTrace(context, "scene", context.scene_id, 0);
    switch (context.scene_id) {
        case kSceneTitle:
            TitleSceneEnter(context);
            break;
        case kSceneExplore:
            ExploreSceneEnter(context);
            break;
        case kSceneDialogue:
            DialogueSceneEnter(context);
            break;
        case kSceneBattle:
            BattleSceneEnter(context);
            break;
        case kSceneMenu:
            MenuSceneEnter(context);
            break;
        default:
            EndingSceneEnter(context);
            break;
    }
    context.dirty = true;
}

void PushDialogue(GameContext& context, uint8_t node_id, uint8_t return_kind) {
    context.dialogue_return = return_kind;
    DialogueStart(context.dialogue, context.strings, node_id);
    PushScene(context, kSceneDialogue);
}

void PushBattle(GameContext& context, uint8_t battle_id) {
    LogTrace(context, "battle", battle_id, 0);
    BattleBegin(context.battle, context.progress, battle_id, context.rng);
    PushScene(context, kSceneBattle);
}

bool StoreProgress(GameContext& context) {
    ProgressStore(context.progress, context.app.storage());
    context.has_save = true;
    return true;
}

void EnterScriptStep(GameContext& context) {
    if (context.progress.script_index >= ScriptCount()) {
        PushScene(context, kSceneEnding);
        return;
    }
    const ScriptStepDef& step = ScriptStep(context.progress.script_index);
    LogTrace(context, "step", context.progress.script_index, static_cast<int32_t>(step.kind));
    switch (step.kind) {
        case StepKind::kDialogue:
            PushDialogue(context, step.arg, kDialogueToScript);
            break;
        case StepKind::kExplore: {
            // A chapter opens at a rest point: companions recruited by a choice
            // join now, and the party wakes up fully healed.
            PartySyncFromFlags(context.progress);
            PartyRestore(context.progress);
            if (context.progress.map_id == step.arg) {
                WorldRestore(context.world, step.arg, context.progress.player_x, context.progress.player_y);
            } else {
                WorldEnter(context.world, step.arg);
                context.progress.map_id = step.arg;
                context.progress.player_x = context.world.x;
                context.progress.player_y = context.world.y;
            }
            PushScene(context, kSceneExplore);
            break;
        }
        case StepKind::kBattle:
            PushBattle(context, step.arg);
            break;
        default:
            PushDialogue(context, EndingDialogueFor(context.progress.flags), kDialogueToEnding);
            break;
    }
}

void AdvanceScript(GameContext& context) {
    if (context.progress.script_index < ScriptCount()) {
        ++context.progress.script_index;
    }
    PartySyncFromFlags(context.progress);
    StoreProgress(context);
    EnterScriptStep(context);
}

namespace {

// 30 Hz drives the typewriter, tile stepping and the battle timers.
inline constexpr uint32_t kTickMs = 33U;

class PalApp final {
   public:
    explicit PalApp(micropixel::Application& application)
        : app_(application),
          renderer_(app_.renderer()),
          info_(renderer_.info()),
          scene_(renderer_.CreateScene(theme::kInk).value()),
          root_(scene_.CreateContainer({.z_order = 0}).value()),
          view_(renderer_, app_.log(), scene_, root_),
          context_(app_, ids::ForLocale(app_.localization().CurrentLocale()), app_.input().info(),
                   BuildLayout(info_), scene_, root_, view_, micropixel::XorShift32{app_.random().U32()}) {}

    int Run() {
        TitleSceneEnter(context_);
        RenderCurrent();

        micropixel::Timer ticker = app_.timers().Every(micropixel::Duration::Milliseconds(kTickMs)).value();
        app_.Run([this, &ticker](const micropixel::Event& event) {
            if (event.TimerFrom(ticker) != nullptr) {
                Tick(kTickMs);
            } else if (const micropixel::TouchEvent* touch = event.touch()) {
                if (!event.gamepad_handled()) {
                    Touch(*touch);
                }
            } else if (const micropixel::KeyEvent* key = event.key()) {
                if (key->phase() == micropixel::KeyPhase::kDown) {
                    Key(key->code());
                }
            } else if (event.type() == micropixel::EventType::kResume) {
                // The panel may have been re-created (rotation, safe-area change).
                context_.refresh_layout = true;
                context_.dirty = true;
            }
            return micropixel::EventResult::kContinue;
        });
        return 0;
    }

   private:
    void Tick(uint32_t delta_ms) {
        if (context_.refresh_layout) {
            context_.refresh_layout = false;
            context_.layout = BuildLayout(renderer_.info());
            context_.dirty = true;
        }
        elapsed_ms_ += delta_ms;
        context_.progress.play_seconds = elapsed_ms_ / 1000U;

        switch (context_.scene_id) {
            case kSceneTitle:
                TitleSceneUpdate(context_, delta_ms);
                break;
            case kSceneExplore:
                ExploreSceneUpdate(context_, delta_ms);
                break;
            case kSceneDialogue:
                DialogueSceneUpdate(context_, delta_ms);
                break;
            case kSceneBattle:
                BattleSceneUpdate(context_, delta_ms);
                break;
            case kSceneMenu:
                MenuSceneUpdate(context_, delta_ms);
                break;
            default:
                EndingSceneUpdate(context_, delta_ms);
                break;
        }
        if (context_.dirty) {
            context_.dirty = false;
            RenderCurrent();
        }
    }

    void Touch(const micropixel::TouchEvent& touch) {
        switch (context_.scene_id) {
            case kSceneTitle:
                (void)TitleSceneTouch(context_, touch);
                break;
            case kSceneExplore:
                (void)ExploreSceneTouch(context_, touch);
                break;
            case kSceneDialogue:
                (void)DialogueSceneTouch(context_, touch);
                break;
            case kSceneBattle:
                (void)BattleSceneTouch(context_, touch);
                break;
            case kSceneMenu:
                (void)MenuSceneTouch(context_, touch);
                break;
            default:
                (void)EndingSceneTouch(context_, touch);
                break;
        }
        // Rendering is left to the next tick. Presenting from inside the input
        // event raced the Host's own present of the previous frame, which made a
        // freshly pushed scene (the status menu) come back rejected.
        context_.dirty = true;
    }

    // A scene whose frame the Host refuses would otherwise trap the player on a
    // stale screen with no way back. Drop out of it instead.
    void AbandonUnpresentableMenu() {
        if (context_.scene_id == kSceneMenu && context_.view.last_frame_rejected()) {
            PushScene(context_, context_.previous_scene);
        }
    }

    void Key(micropixel::KeyCode code) {
        switch (context_.scene_id) {
            case kSceneTitle:
                (void)TitleSceneKey(context_, code);
                break;
            case kSceneExplore:
                (void)ExploreSceneKey(context_, code);
                break;
            case kSceneDialogue:
                (void)DialogueSceneKey(context_, code);
                break;
            case kSceneBattle:
                (void)BattleSceneKey(context_, code);
                break;
            case kSceneMenu:
                (void)MenuSceneKey(context_, code);
                break;
            default:
                (void)EndingSceneKey(context_, code);
                break;
        }
        context_.dirty = true;
    }

    void RenderCurrent() {
        switch (context_.scene_id) {
            case kSceneTitle:
                TitleSceneRender(context_);
                break;
            case kSceneExplore:
                ExploreSceneRender(context_);
                break;
            case kSceneDialogue:
                DialogueSceneRender(context_);
                break;
            case kSceneBattle:
                BattleSceneRender(context_);
                break;
            case kSceneMenu:
                MenuSceneRender(context_);
                break;
            default:
                EndingSceneRender(context_);
                break;
        }
    }

    micropixel::Application& app_;
    micropixel::Renderer renderer_;
    micropixel::RendererInfo info_;
    micropixel::Scene scene_;
    micropixel::ContainerNode root_;
    GameView view_;
    GameContext context_;
    uint32_t elapsed_ms_{};
};

}  // namespace

int PalAppMain() {
    micropixel::Application app;
    PalApp game(app);
    app.log().Info("pal: Chinese Paladin ready");
    return game.Run();
}

}  // namespace pal
