# Third-party notices

## This project

仙剑奇侠传 (Chinese Paladin) is a turn-based RPG built on the MicroPixel C++
SDK. All game code, content tables, dialogue and procedural vector art in this
project were written for this app.

> 仙剑奇侠传 is the title of a commercial game published by Softstar
> Entertainment. This is an unaffiliated, non-commercial study/port exercise;
> no code, data, art or script from that title is used here.

## Design references

The systems and code organization follow these open-source projects as
*design references only* — no source code or assets were copied from either:

- [sdlpal/sdlpal](https://github.com/sdlpal/sdlpal) — the SDL remake of
  仙剑奇侠传 (grid exploration, turn-based battle flow, 五灵 spell model).
- MicroPixel's own `guest/apps` examples (`sdk-demo`, `tomb-explorer`).

## MicroPixel SDK

This app is compiled against the MicroPixel SDK, which is distributed
separately under its own license:

- MicroPixel SDK 0.20.1 — https://github.com/78/micropixel

The SDK's `guest/sdk`, `guest/runtime` and `guest/abi` trees are linked into
every Bundle and remain under their upstream terms. See the SDK's own
`THIRD_PARTY_NOTICES.md` for the toolchain (LLVM/clang, wasi-libc, WAMR)
notices.

## Previous revision

An earlier revision of this app adapted the tile-merging mechanics of
[ArminNeyrizi/2048-cpp](https://github.com/ArminNeyrizi/2048-cpp) for a 2048
mini-game. That mode has been removed, so the MIT notice that used to live in
this file no longer applies.
