---
name: micropixel-publish
description: '把 App 发布到 MicroPixel 应用商店：店招资料（截图、简介、玩法说明）、版本号规则、publish --dry-run 与 auth github、发布后编辑。Use when publishing to the store, preparing store screenshots and descriptions, releasing an App version, 发布应用, 上架, 商店资料, 截图准备.'
argument-hint: '[app 路径]'
---

# 发布到 MicroPixel 应用商店

## When to Use

- 某个 app 开发完成，要发布或更新到商店
- 要准备商店资料（截图、简介、玩法说明）
- 发布被拒/上传失败，需要知道硬性限制

## 硬性限制（来自文档索引，2026-10）

| 项 | 值 |
| --- | --- |
| 包体积上限 | 8 MiB |
| 单张截图 | ≤ 2 MiB，PNG / JPEG |
| 商店截图数量 | ≥ 2 张实际运行画面，最多 8 张 |
| **封面不计入那 2 张** | 封面是启动资产，不能当功能截图 |
| 同一版本换 Bundle | **不允许**，更新必须先提升 `app.json` 的 `version` |

## Procedure

### 1. 本地自检

```powershell
.\scripts\preflight.ps1 -AppPath apps\<slug> -RunVerification
```

它会检查：构建是否干净、该 app 自己的验证脚本是否通过、SPDX 头是否齐全、
`LICENSE` / `NOTICE` 是否在、根 README 是否登记了这个 app、`app.json` 的版本号。
任何一项失败都先修掉再继续。

### 2. 准备商店资料

按"主要界面 + 核心玩法"各截一张真实运行图（在不同状态下截，中间要真的操作 app 切换画面）：

```powershell
mkdir store
micropixel --port COM3 screenshot --output store/01-main.png
# 在设备上操作，进入另一个状态后再截
micropixel --port COM3 screenshot --output store/02-playing.png
```

- `store/` 建议进版本库（它不属于 `.gitignore` 里的 `build/`、`screenshots/`）
- 介绍文字写 `store/description.md`：**CLI 不会自动读取或上传它**，是给人维护用的
- 资料要覆盖文档要求的四项：App 是什么 / 能做什么 / 操作方式 / 目标与胜负或完成条件

### 3. 发布

```powershell
micropixel publish --dry-run     # 只做本地正式构建与校验，不登录、不上传
micropixel auth github           # 浏览器授权 CLI
micropixel publish               # 上传正式 Bundle，返回 url 与 editUrl
```

- 打开 `editUrl` 填简介/详细说明/玩法说明，上传截图后保存
- 只改介绍或截图时在网页保存即可，**不必重发 Bundle**

### 4. 更新

1. 提升 `app.json` 的 `version`
2. `micropixel build` + 该 app 的验证脚本跑一遍
3. `micropixel publish`

## 发布前对着本仓库逐项确认

- [ ] `apps/<slug>/AGENTS.md` 里的验证脚本全绿（pal 是 `tools/run-battle-sim.ps1`，
      `smart` 与 `skills-no-items` 必须 100%）
- [ ] `app.json` 的 `requirements` 与实际实现一致（多声明会挡设备，少声明会异常）
- [ ] 新增源文件有 `// SPDX-License-Identifier: MIT`
- [ ] 内容声明仍然成立：只用本仓库生成的素材（脚本生成的图、运行期合成的音频），
      没有外部素材；`NOTICE` 与实际一致
- [ ] 根 `README.md` 的状态表已更新

## 官方文档

- AI 文档索引：https://micropixel.ai/docs/ai/index.json
- 发布应用（资料模板与限制）：https://micropixel.ai/docs/ai/content/publishing.md
- 资源与 Bundle：https://micropixel.ai/docs/ai/content/assets-and-bundles.md
- 限制与兼容性：https://micropixel.ai/docs/ai/content/reference.md
