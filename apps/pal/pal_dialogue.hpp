// SPDX-License-Identifier: MIT
// Dialogue runtime: a cursor over the authored node graph plus the typewriter
// reveal. Text stays in the generated localization catalog; only ids are stored
// here so a running dialogue is a handful of bytes.

#ifndef PAL_DIALOGUE_HPP
#define PAL_DIALOGUE_HPP

#include <stdint.h>

#include "pal_content.hpp"
#include "pal_model.hpp"

namespace pal {

// One revealed codepoint every kRevealMsPerCodepoint milliseconds.
inline constexpr uint32_t kRevealMsPerCodepoint = 40U;

struct DialogueState final {
    bool active{};
    uint8_t node{};
    uint8_t line{};
    uint8_t selected{};       // highlighted choice
    bool choosing{};
    uint32_t reveal_ms{};     // elapsed reveal time for the current line
    uint32_t line_codepoints{};
};

// --- UTF-8 helpers (the whole script is Chinese, so text is multi-byte) -----

uint32_t Utf8Count(const char* text);
uint32_t Utf8PrefixBytes(const char* text, uint32_t codepoints);
// Writes at most `capacity - 1` bytes of the first `codepoints` codepoints.
void Utf8CopyPrefix(char* out, uint32_t capacity, const char* text, uint32_t codepoints);

// --- runtime ----------------------------------------------------------------

void DialogueStart(DialogueState& state, const ids::Catalog& strings, uint8_t node_id);
void DialogueUpdate(DialogueState& state, uint32_t delta_ms);

// Codepoints currently visible on the active line.
uint32_t DialogueRevealed(const DialogueState& state);
bool DialogueLineDone(const DialogueState& state);

// Confirm press. Reveals the rest of a partially shown line first; returns true
// once the whole dialogue has been dismissed.
bool DialogueConfirm(DialogueState& state, const ids::Catalog& strings);

// Left/right on the choice list.
void DialogueMove(DialogueState& state, int32_t delta);

// Applies the highlighted choice to `flags`. The choice either ends the
// dialogue (state.active becomes false) or branches to another node.
void DialoguePick(DialogueState& state, const ids::Catalog& strings, uint32_t& flags);

const DialogueNodeDef& DialogueCurrent(const DialogueState& state);

}  // namespace pal

#endif  // PAL_DIALOGUE_HPP
