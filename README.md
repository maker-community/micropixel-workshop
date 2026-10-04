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

## 环境基线

| 项 | 版本 |
| --- | --- |
| MicroPixel SDK | 0.20.1 |
| 设备固件 | 0.9.5（SenseCAP Watcher，ESP32-S3，412×412 圆屏） |
| 工具链 | wasi-sdk clang++，`--target=wasm32-wasip1 -std=c++23 -ffreestanding` |
| AOT | target `xtensa`，WAMR 格式 v6 |

## 构建与运行

每个 app 自包含，**CLI 必须在 app 自己的目录里运行**（`app.json` 所在层）：

```powershell
cd apps/pal
micropixel build
powershell -NoProfile -File tools\run-battle-sim.ps1 -Runs 300
micropixel --port COM3 run --aot-target xtensa --no-follow
```

`micropixel` 在 `C:\Users\gil\AppData\Local\MicroPixel\bin\micropixel.exe`，不一定在 PATH 里。
每个 app 的验证流程、坐标表和已知坑写在它自己的 `AGENTS.md` 里。

## 目录结构

```
apps/<app>/        一个完整的 app 工程：app.json、源码、assets/、audio/、i18n/、scenes/、tools/
apps/<app>/AGENTS.md
                   该 app 的构建-验证流程与踩坑记录（改代码前先读）
AGENTS.md          仓库级规则：许可、提交、目录约定
NOTICE             第三方组件 + 内容声明
```

跨 app 的 `docs/` 与 `tools/` 等第二个 app 落地后再抽，先不做过度设计。

## 许可

- **代码：MIT**（见 [`LICENSE`](LICENSE)）。新增源文件第一行写 `// SPDX-License-Identifier: MIT`。
- **第三方与内容声明**：[`NOTICE`](NOTICE)。MicroPixel SDK 是 Apache-2.0，在这里只作为构建期依赖；
  本仓库为非官方同人作品，未包含任何商业作品的素材。
- **单个 app 的第三方说明**放在 `apps/<app>/THIRD_PARTY_NOTICES.md`；若某个 app 需要不同的许可
  （例如引入 GPL 代码），它必须带自己的 `LICENSE`，并在本节的例外列表里登记。
