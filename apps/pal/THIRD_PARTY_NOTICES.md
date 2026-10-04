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

### SDLPAL license boundary (GPL-3.0)

SDLPAL is licensed under the GNU General Public License v3.0. It is used here
the way a design document is used: the damage curves, the dexterity-based action
order, elemental resistance and level-gated spell learning were read,
understood, and then re-implemented independently in this project's own C++23
sources. Ideas and formulas are not copyrightable; their expression is.

Rules that anyone extending this app has to respect:

- **Nothing may be copied or translated out of SDLPAL** — not a function, not a
  comment, not a file under its `docs/`. Copying any part of it would relicense
  this entire app (sources and Bundle) under GPL-3.0, which is not the license
  this project is distributed under (see the SPDX headers: Apache-2.0).
- SDLPAL contains no data from the original commercial game either; it requires
  the user to supply those files separately, so nothing here was obtained
  through it.
- [gramlib/sdlpal-wasm](https://github.com/gramlib/sdlpal-wasm) was reviewed and
  **not** used. The repository carries no license file, and its payload is a wasm
  build derived from GPL-3.0 SDLPAL. The official SDLPAL tree already ships its
  own `emscripten/` web target, so that wrapper adds no engine code anyway.

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
