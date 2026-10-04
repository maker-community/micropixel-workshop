---
name: micropixel-run-on-device
description: '把 App 构建/安装/启动到真实 MicroPixel 设备上并验证它真的在跑：COM 端口、run --no-follow、截图与按键/点击语义、逐像素活性检查、一次最多 16 个输入的批量下发。Use when flashing to the device, running on hardware, taking screenshots, driving input press/tap/sequence, debugging a frozen or black screen, 烧录, 上机, 设备验证, 冻屏, 黑屏.'
argument-hint: '[app 路径或要验证的行为]'
---

# 在真实设备上运行与验证

## When to Use

- 要把某个 app 装到设备上看效果，或验证一个改动在真机上的行为
- 画面看着不对（黑屏、冻屏、花屏），需要判断"是渲染挂了"还是"逻辑还在跑"
- 需要脚本化地驱动输入（按键/点击）并留证据

## 铁律（都是踩过的坑）

1. **每条命令都带 `--port COM3`**。`micropixel port list` 在本机返回 0 个端口，
   别依赖自动发现。设备是 CH342 桥，控制口 COM3、固件口 COM4。
2. **`micropixel logs` 永远不会自己返回**（它跟随日志）。要么放后台 job 加超时，
   要么异步终端跑完直接 kill。想要日志就 `--screenshot` 之外另存或用 `Tee-Object` 落文件再另开命令读。
3. **`app last-error` 可能是上一次的（stale）**。别单独拿它下结论，要和"刚发生的现象"对齐时间。
4. **截图默认是"你这次输入之前的那一帧"**。想确认某个操作的效果，必须**操作后再截一张**，
   别用同一张图判断"点了没反应"。
5. **冻屏 ≠ 死机**：缺字导致的冻屏里，逻辑继续跑、计时器继续走、只有画面停在旧帧。
   判断依据是逐像素比对，不是肉眼。

## Procedure

### 1. 构建并上机

```powershell
cd apps/<slug>                                   # CLI 必须在 app 目录里跑
micropixel build                                 # 先单独构建，零 warning 才算过
micropixel --port COM3 run --aot-target xtensa --no-follow
```

- `--aot-target xtensa` 是 ESP32-S3 用的；`--no-follow` 表示装完启动就返回，
  不加它会跟随日志并"卡住"（见铁律 2）。
- 只想装不启动：`micropixel --port COM3 app install <bundle>` / `app start <app_id>`。
- 确认当前前台是哪个 app：`micropixel --port COM3 app list`，看 `active` / `lifecycle` 字段。

### 2. 抓一帧看现状

```powershell
micropixel --port COM3 screenshot --output "$env:TEMP\shot.png"
```
然后用看图工具直接看。**需要"变了没有"的结论时不要靠眼睛**，跑活性检查：

```powershell
.\scripts\liveness-check.ps1                       # 默认 COM3：截一帧 → 按键 → 再截一帧
.\scripts\liveness-check.ps1 -A a.png -B b.png      # 比较已有的两帧
```

### 3. 驱动输入

```powershell
micropixel --port COM3 input press confirm      # up/down/left/right/confirm/back
micropixel --port COM3 input tap 206 294        # 坐标是按当前屏幕分辨率量的逻辑坐标
micropixel --port COM3 input swipe x1 y1 x2 y2
micropixel --port COM3 input sequence seq.json  # 一次下发多个操作（推荐）
```

- **坐标怎么来**：先截一帧、量出目标控件的像素矩形，取内缩后的中心点。
  已经量好的坐标表见 [`./references/device-input.md`](./references/device-input.md)。
- **点击是位置式的，按键是状态式的**：点击"术法按钮"永远只做一件事；
  而按键序列依赖光标当前在哪，中间多吞一个 press 就会错位。能点击就点击。
- **批量下发有硬上限**：单次 sequence ≤ 16 个操作、≤ 4 张截图、延时总和 ≤ 10s。
  设备端 schema 见 https://micropixel.ai/docs/specs/device-control-v1.schema.json
  （`{"steps":[]}` 这种空数组会被拒 —— 至少要有一步有效操作）。
- 长任务用 Job：`micropixel --port COM3 job wait <job-id>`，配 `--wait-timeout`。

### 4. 卡住/崩溃时

| 现象 | 先看什么 |
| --- | --- |
| 画面不动 | 活性检查（脚本）；0 差异 + 逻辑仍在跑 → 优先怀疑缺字，见 `apps/pal/AGENTS.md` |
| App 退出/重启 | `micropixel --port COM3 app last-error` + 宿主机日志里的 `guest trapped` / `present rejected` |
| 装不上 | 包体积 ≤ 8 MiB、设备上 app 数 ≤ 50、是否停掉了正在运行的 Guest |
| 命令没反应 | 是不是没带 `--port COM3`；是不是上一条 `logs` 还没退出占着串口 |
| `Cannot open USB device COMx` | 端口不存在或被占用：设备没插/没枚举，或上一个 `logs`/`run` 还在占着串口。注意 `micropixel port list` 返回 0 个端口是本机的正常现象，**不能**据此判断设备不在 |

## 证据要求

验证结论要能复现：截图落在 `screenshots/`（已 gitignore），并在报告里写清
**命令 + 现象 + 结论**。只用"看起来正常"不算验证。

## 官方文档

- AI 文档索引：https://micropixel.ai/docs/ai/index.json
- USB 本地开发（COM 端口与排错）：https://micropixel.ai/docs/ai/content/usb-development.md
- 限制与兼容性：https://micropixel.ai/docs/ai/content/reference.md
- Control API（远程/Job）：https://micropixel.ai/docs/specs/control-api-v1.openapi.json
