# AGENTS.md — 仓库级规则

这里只写**跨 app** 的约定。每个 app 的构建、验证流程和该 app 特有的坑，写在
`apps/<app>/AGENTS.md`（例如 [`apps/pal/AGENTS.md`](apps/pal/AGENTS.md)），改代码前先读那一份。

## 目录约定

- 每个 app 都是一个自包含的 MicroPixel 工程：`app.json` 在 `apps/<app>/` 这一层，
  源码、`assets/`、`audio/`、`i18n/`、`scenes/`、`tools/` 全部在它下面。
- **CLI 必须在 app 自己的目录里运行**（`micropixel build` / `run` 以 `app.json` 所在目录为准），
  不要从仓库根目录跑。
- 不要把某个 app 的源码拆到 app 目录之外。要共享代码之前先确认 SDK 的构建支持额外的
  include 路径，没验证之前宁可各自复制小工具函数。
- app 目录名 == `app_id` 最后一段（`com.maker.pal` → `apps/pal`）。SDK 用这一段生成产物名
  （`pal_strings.hpp` / `build/pal.aot`），所以改了要同步 `#include` 和 `namespace ids =`。

## 提交前

- `micropixel build` 干净通过：`-Wall -Wextra -Werror`，未使用的参数/变量会直接编译失败。
- 跑该 app 自己的验证脚本（pal 是 `tools/run-battle-sim.ps1`，它同时负责内容校验和平衡模拟）。
- 新增源文件第一行写 `// SPDX-License-Identifier: MIT`（`.ps1` 用 `#`）。
- **`.ps1` 一律只写 ASCII**（注释和信息都用英文）：Windows PowerShell 5.1 不认没有 BOM 的
  UTF-8，中文会被读成乱码、甚至吃掉引号终止符，报出莫名其妙的"字符串缺少终止符"。
  中文说明写在 `.md` 里（含 skill 的 references），脚本里只留命令和英文输出。
- 反过来，脚本**读**仓库里的文件时要显式按 UTF-8 解码：
  `[System.IO.File]::ReadAllText($p, [System.Text.Encoding]::UTF8)`。
  用 `Get-Content -Raw` 读带中文的 `app.json` 会被 ANSI 解码破坏引号配对，
  `ConvertFrom-Json` 只会报一个没有上下文的 ArgumentException。
- 提交信息用英文；`build/`、`screenshots/`、`devlog.txt`、`.env`、`micropixel.lock.json`
  已在 `.gitignore` 里，不要强加。

## 设备

- 设备：SenseCAP Watcher（ESP32-S3，412×412 圆屏，固件 0.9.5），USB-C 的 CH342 桥。
- **每条命令都要带 `--port COM3`**；`micropixel port list` 返回 0 个端口。
- `micropixel logs` **永远不会自己返回**：包在后台 job 里加超时，或放异步终端跑完就 kill。
- `app last-error` 看崩溃原因，但信息可能是上一次的（stale）。
- 截图/输入的具体语义（`input tap` 抓的是点击前那一帧、提示条来不及截、近似帧要逐像素比）
  见 `apps/pal/AGENTS.md` 的设备与坐标两节，那套经验对所有 app 都适用。

## 许可

- 仓库代码是 **MIT**；第三方与内容声明见 [`NOTICE`](NOTICE)。
- 不要把 GPL 代码并入某个 app —— 那会让该 app 无法继续用 MIT，必须单独带 `LICENSE` 并登记例外。
