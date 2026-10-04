# app.json 速查（本仓库约定 + 文档摘要）

> 权威来源：`micropixel init` 生成的清单 + https://micropixel.ai/docs/ai/content/quickstart.md
> 与 https://micropixel.ai/docs/ai/content/reference.md 。字段有疑问时先跑 `micropixel init` 生成一份对照。
> 下面以 `apps/pal/app.json` 为实例。

## 必填骨架

```json
{
  "schema_version": 1,
  "app_id": "com.maker.<slug>",
  "version": "0.1.0",
  "threading": "none",
  "title": { "default": "zh-CN", "values": { "zh-CN": "显示名" } },
  "localization": { "default": "zh-CN", "translations": { "zh-CN": "i18n/zh-CN.json" } },
  "sources": ["main.cpp", "scenes/title_scene.cpp"],
  "asset_manifest": "assets/manifest.json",
  "launch_asset": "launch",
  "requirements": { "...": "..." }
}
```

要点：

- **`version` 只有发布后才需要严格对待**：同一版本不能再传一个不同的 Bundle，更新必须先提版本。
- **`sources` 必须列全所有 `.cpp`**；漏掉的源文件不会参与构建，症状是"函数找不到定义"或功能静默缺失。
- **路径都是相对 app 目录的**，所以整个 app 目录可以整体搬家（本仓库就是 `git mv` 到 `apps/<slug>/` 的）。
- `launch_asset` 指向 `assets/manifest.json` 里的条目名（封面），封面尺寸与大小限制见
  skill `micropixel-publish`。

## requirements（能力声明）

声明要与实现一致：多声明会白白挡住装不上的设备，少声明会在真机上被拒或行为异常。

| 字段 | 用途 | 本仓库实例 |
| --- | --- | --- |
| `system_font` | 系统字体语种：`en` / `zh-CN` / `zh-TW` / `ja-JP` / `ko-KR`，需要相应固件 | `zh-CN` |
| `display.layouts` | 支持的屏幕形态 | `square` / `portrait` / `landscape` |
| `display.min_width/min_height` | 低于此尺寸不安装 | `320` |
| `required` | 缺了就不能跑的能力 | `input.touch` |
| `optional` | 有更好、没有降级 | `audio.output`、`input.keys` |
| `services` | 需要的外部服务 | 目前为空 |

写代码时的对应关系（本仓库经验）：

- 用了 `AudioDirector` / `ToneSequencer` → `optional` 里要有 `audio.output`，
  且实现要在宿主报告无音频时降级为静默 no-op（能力是可选的，不能崩）。
- 依赖方向键/环带按键 → `optional` 里加 `input.keys`，同时保留触摸通路。
- `system_font` 只影响宿主提供哪套字形；**缺字会导致冻屏**，见
  `apps/pal/AGENTS.md` 的缺字一节。

## 当前平台限制（2026-10，来自文档索引）

| 项 | 值 |
| --- | --- |
| 单个包体积上限 | 8 MiB |
| 一台设备最多安装 app 数 | 50 |
| 单次 `input sequence` 操作数 | ≤ 16 |
| 单次 sequence 延时总和 | ≤ 10000 ms |
| 单次 sequence 截图数 | ≤ 4 |
| AOT 格式 | v6（WAMR fork，wamrc 2.4.3） |

版本对上号：本仓库基线写的是 SDK 0.20.1，文档索引当前为 **0.20.2**，升级前先读
`/docs/ai/content/reference.md` 的兼容性说明（历史上 SDK 0.14.0 删过
`SurfaceNode` / `Container::CreateSurfaceNode`，0.18.0 才引入系统字体）。
