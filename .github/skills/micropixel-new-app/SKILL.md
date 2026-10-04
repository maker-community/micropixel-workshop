---
name: micropixel-new-app
description: '在本仓库新建一个 MicroPixel app（游戏或小工具）：脚手架、app_id 与目录命名、app.json 能力声明、首次构建。Use when adding a new app, scaffolding a new MicroPixel App, running micropixel init, creating a new game for the MicroPixel host, 新增 app, 新建应用, 新游戏, 加一个小工具.'
argument-hint: '[玩法或用途一句话]'
---

# 新建一个 MicroPixel app

## When to Use

- 要在 `apps/` 下加一个新的 app（游戏、小工具、玩法实验）
- 要把 SDK 自带示例（`sdk-demo` / `snake` / `maze-evil` / `blocks` / `tilt`）搬进本仓库
- 不确定 `app_id`、目录名或能力声明（`requirements`）该怎么写

## 前置

- `micropixel` CLI 可用。它不一定在 PATH 里：
  `C:\Users\gil\AppData\Local\MicroPixel\bin\micropixel.exe`
- 要上设备时，命令一律带 `--port COM3`（本机 `micropixel port list` 返回 0 个端口）。
  细节与输入方式见 skill `micropixel-run-on-device`。

## 命名规则（仓库约定，定了就别改）

| 项 | 规则 | 例子 |
| --- | --- | --- |
| `app_id` | `com.maker.<slug>` | `com.maker.arena` |
| 目录 | `apps/<slug>`，slug == `app_id` 最后一段 | `apps/arena` |
| 产物名 | SDK 用 slug 生成 | `arena_strings.hpp` / `build/arena.aot` |

slug 一旦发布就不能改：改了要同步源码里的 `#include "<slug>_strings.hpp"` 和 `namespace ids = ...`。

## Procedure

1. 生成脚手架（在**仓库根**执行，注意是 init 到 `apps/` 下）：
   `micropixel init apps/<slug> --app-id com.maker.<slug> --title "<显示名>"`
   目标目录里已有 `app.json` 时 init 会拒绝覆盖；已有源码可用 `micropixel init apps/<slug>` 让它扫描源码生成清单。
2. 校对 `app.json`，见 [`./references/app-json.md`](./references/app-json.md)：
   - `sources` 必须列全所有 `.cpp`
   - `requirements` 声明的能力要与实现一致：用了音频就写 `audio.output`，用了方向键就写 `input.keys`，
     只声明真正需要的，别默认全开
3. **目录自包含**：源码、`assets/`、`audio/`、`i18n/`、`scenes/`、`tools/` 都在 `apps/<slug>/` 下。
   不要把这个 app 的源码拆到 app 目录之外（SDK 构建只认 app 目录内的路径）。
4. 新增源文件第一行写 `// SPDX-License-Identifier: MIT`（`.ps1` 用 `#`，`.mjs` 用 `//`）。
5. 构建：`cd apps/<slug>` 后执行 `micropixel build`。
   **CLI 必须在 app 自己的目录里跑**（以 `app.json` 所在目录为准），必须零 warning
   （`-Wall -Wextra -Werror`，未使用的参数/变量会直接编译失败）。
6. 写 `apps/<slug>/AGENTS.md`：这个 app 的构建-验证流程、坐标表、自己的坑。
   仓库级规则（许可、提交、目录约定）在根 `AGENTS.md`，不要重复。
7. 在根 `README.md` 的 Apps 表加一行：app / 说明 / 状态。
8. 上设备验证 → skill `micropixel-run-on-device`。发布 → skill `micropixel-publish`。

## 官方文档（改动前先拉最新，别凭记忆写）

- **AI 文档索引（机器可读，先读它）**：https://micropixel.ai/docs/ai/index.json
  —— 里面有当前 SDK 版本、各页 markdown 地址、能力矩阵、当前限制
- 快速入门：https://micropixel.ai/docs/ai/content/quickstart.md
- 游戏开发指南（规则/输入/画面/音效/存档）：https://micropixel.ai/docs/ai/content/app-development.md
- 资源与 Bundle：https://micropixel.ai/docs/ai/content/assets-and-bundles.md
- SDK API：https://micropixel.ai/docs/ai/content/sdk.md
- 限制与兼容性：https://micropixel.ai/docs/ai/content/reference.md
- SDK 自带示例（抄结构最快）：SDK 安装目录下 `guest/apps/<name>`，
  CLI 也能直接用 `micropixel build path/to/app` 指定路径构建

## 完成标准

- [ ] `micropixel build` 干净通过（零 warning）
- [ ] `app.json` 的 `sources` / `requirements` 与实际实现一致
- [ ] 所有新增源文件有 SPDX 头
- [ ] `apps/<slug>/AGENTS.md` 已写，根 `README.md` 状态表已加行
