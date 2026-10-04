# micropixel-workshop

Games and small apps for the MicroPixel host — built with the MicroPixel SDK
(C++23 → `wasm32-wasip1`, AOT on device).

MicroPixel 宿主的游戏与小应用合集。每个 app 都是独立可构建的工程（各自有 `app.json`），
放的是能跑在设备上的成品，不是 SDK 示例代码。

## Apps

| app | 说明 | 状态 |
| --- | --- | --- |
| [`apps/pal`](apps/pal) | 回合制仙侠 RPG：原创重制，致敬国产单机时代的仙侠题材。15 张地图 / 17 场战斗 / 67 步剧本，含五灵相克、技能解锁、存档迁移与一个平衡模拟器 | 可通关，已上机 |

新增 app 时在这里加一行。

## 快速开始

第一次拿到这个仓库，看 [`docs/getting-started.md`](docs/getting-started.md)（完整路径：装 CLI → 构建 →
不用设备也能验证 → 上真机 → 让 AI 接手）。最短路径：

```powershell
git clone https://github.com/maker-community/micropixel-workshop.git
cd micropixel-workshop/apps/pal
micropixel build
powershell -NoProfile -File tools\run-battle-sim.ps1 -Runs 300
```

前提：已安装 MicroPixel SDK 与 CLI（https://micropixel.ai/docs/environment/）。
**没有设备也能跑上面两条**：第二条是整条剧本的 300 遍模拟 + 内容校验，
`smart` / `skills-no-items` 显示 `300/300` 即为通过。

## 环境基线

| 项 | 版本 |
| --- | --- |
| MicroPixel SDK | 记录基线 0.20.1（本机 CLI 实测 `micropixel --version` = 0.20.1；官方 SDK 已有 0.20.2，升级前读兼容性说明） |
| 设备固件 | 0.9.5（SenseCAP Watcher，ESP32-S3，412×412 圆屏） |
| 工具链 | wasi-sdk clang++，`--target=wasm32-wasip1 -std=c++23 -ffreestanding` |
| AOT | target `xtensa`，WAMR 格式 v6 |

## 构建与运行

每个 app 自包含，**CLI 必须在 app 自己的目录里运行**（`app.json` 所在层）：

```powershell
cd apps/pal
micropixel build
powershell -NoProfile -File tools\run-battle-sim.ps1 -Runs 300
micropixel --port COM3 run --aot-target xtensa --no-follow   # COM3 是本机端口，换成你的
```

- `micropixel` 装好后在 PATH 里（本机为 `C:\Users\gil\AppData\Local\MicroPixel\bin\micropixel.exe`）。
- 前两条**不需要设备**；第三条要设备，串口号按你的机器改。
  注意：`micropixel port list` 在本机的 CH342 桥上返回 **0 个端口**（它只认 ESP32 原生 VID），
  不能据此判断设备不在；串口号用设备管理器看，见 `docs/getting-started.md` 第 4 章。
- 每个 app 的验证流程、坐标表和已知坑写在它自己的 `AGENTS.md` 里。

## 目录结构

```
apps/<app>/                一个完整的 app 工程：app.json、源码、assets/、audio/、i18n/、scenes/、tools/
apps/<app>/README.md       这个 app 是什么、怎么玩、怎么改（面向人）
apps/<app>/AGENTS.md       该 app 的构建-验证流程与踩坑记录（改代码前先读）
docs/getting-started.md    第一次拿到仓库的完整上手路径
.github/skills/            AI 工作流（新建 app / 上机验证 / 发布），可当斜杠命令用
AGENTS.md                  仓库级规则：许可、提交、目录约定
NOTICE                     第三方组件 + 内容声明
```

## 和 AI 一起用

规则和技能都在版本库里，克隆下来就能用，不需要额外配置：

| 你想做什么 | 对 AI 说 |
| --- | --- |
| 新加一个 app | `/micropixel-new-app` |
| 上机运行、验证、排查冻屏/黑屏 | `/micropixel-run-on-device` |
| 发布到商店、准备截图与说明 | `/micropixel-publish` |

AI 会自动读根 `AGENTS.md`、`apps/<app>/AGENTS.md` 与按需加载的 `.github/skills/`，
所以你不必先解释项目背景。

## 许可

- **代码：MIT**（见 [`LICENSE`](LICENSE)）。新增源文件第一行写 `// SPDX-License-Identifier: MIT`。
- **第三方与内容声明**：[`NOTICE`](NOTICE)。MicroPixel SDK 是 Apache-2.0，在这里只作为构建期依赖；
  本仓库为非官方同人作品，未包含任何商业作品的素材。
- **单个 app 的第三方说明**放在 `apps/<app>/THIRD_PARTY_NOTICES.md`；若某个 app 需要不同的许可
  （例如引入 GPL 代码），它必须带自己的 `LICENSE`，并在本节的例外列表里登记。
