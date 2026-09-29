// SPDX-License-Identifier: Apache-2.0
// Dialogue cursor and UTF-8 prefix helpers.

#include "pal_dialogue.hpp"

namespace pal {
namespace {

inline bool IsContinuationByte(char byte) { return (static_cast<uint8_t>(byte) & 0xC0U) == 0x80U; }

}  // namespace

uint32_t Utf8Count(const char* text) {
    if (text == nullptr) {
        return 0U;
    }
    uint32_t count = 0U;
    for (const char* cursor = text; *cursor != '\0'; ++cursor) {
        if (!IsContinuationByte(*cursor)) {
            ++count;
        }
    }
    return count;
}

uint32_t Utf8PrefixBytes(const char* text, uint32_t codepoints) {
    if (text == nullptr) {
        return 0U;
    }
    uint32_t seen = 0U;
    uint32_t index = 0U;
    while (text[index] != '\0') {
        if (!IsContinuationByte(text[index])) {
            if (seen == codepoints) {
                return index;
            }
            ++seen;
        }
        ++index;
    }
    return index;
}

void Utf8CopyPrefix(char* out, uint32_t capacity, const char* text, uint32_t codepoints) {
    if (out == nullptr || capacity == 0U) {
        return;
    }
    uint32_t length = Utf8PrefixBytes(text, codepoints);
    if (length > capacity - 1U) {
        length = capacity - 1U;
        // Never cut a multi-byte sequence in half.
        while (length > 0U && IsContinuationByte(text[length])) {
            --length;
        }
    }
    for (uint32_t index = 0U; index < length; ++index) {
        out[index] = text[index];
    }
    out[length] = '\0';
}

void DialogueStart(DialogueState& state, const ids::Catalog& strings, uint8_t node_id) {
    state = DialogueState{};
    state.active = true;
    state.node = node_id < kDialogueCount ? node_id : 0U;
    state.line = 0U;
    state.reveal_ms = 0U;
    const DialogueNodeDef& node = DialogueNode(state.node);
    state.line_codepoints = node.line_count > 0U ? Utf8Count(strings.Get(node.lines[0].text)) : 0U;
}

void DialogueUpdate(DialogueState& state, uint32_t delta_ms) {
    if (!state.active || state.choosing) {
        return;
    }
    state.reveal_ms += delta_ms;
}

uint32_t DialogueRevealed(const DialogueState& state) {
    const uint32_t shown = state.reveal_ms / kRevealMsPerCodepoint;
    return shown < state.line_codepoints ? shown : state.line_codepoints;
}

bool DialogueLineDone(const DialogueState& state) { return DialogueRevealed(state) >= state.line_codepoints; }

bool DialogueConfirm(DialogueState& state, const ids::Catalog& strings) {
    if (!state.active) {
        return true;
    }
    if (!DialogueLineDone(state)) {
        // First press fast-forwards the typewriter instead of skipping the line.
        state.reveal_ms = state.line_codepoints * kRevealMsPerCodepoint;
        return false;
    }
    const DialogueNodeDef& node = DialogueNode(state.node);
    const bool last_line = state.line + 1U >= node.line_count;
    if (last_line && node.choice_count > 0U) {
        state.choosing = true;
        state.selected = 0U;
        return false;
    }
    if (!last_line) {
        ++state.line;
        state.reveal_ms = 0U;
        state.line_codepoints = Utf8Count(strings.Get(node.lines[state.line].text));
        return false;
    }
    state.active = false;
    return true;
}

void DialogueMove(DialogueState& state, int32_t delta) {
    if (!state.active || !state.choosing) {
        return;
    }
    const DialogueNodeDef& node = DialogueNode(state.node);
    if (node.choice_count == 0U) {
        return;
    }
    int32_t next = static_cast<int32_t>(state.selected) + delta;
    const int32_t count = static_cast<int32_t>(node.choice_count);
    while (next < 0) {
        next += count;
    }
    state.selected = static_cast<uint8_t>(next % count);
}

void DialoguePick(DialogueState& state, const ids::Catalog& strings, uint32_t& flags) {
    if (!state.active || !state.choosing) {
        return;
    }
    const DialogueNodeDef& node = DialogueNode(state.node);
    if (state.selected < node.choice_count) {
        const ChoiceDef& choice = node.choices[state.selected];
        flags |= choice.set_flags;
        flags &= ~choice.clear_flags;
        state.choosing = false;
        if (choice.next >= 0) {
            // Branch: keep the dialogue running on the new node.
            DialogueStart(state, strings, static_cast<uint8_t>(choice.next));
            return;
        }
    }
    state.active = false;
}

const DialogueNodeDef& DialogueCurrent(const DialogueState& state) { return DialogueNode(state.node); }

}  // namespace pal
