# AGENTS.md

`com.maker.pal`（仙剑奇侠传）—— MicroPixel 宿主的访客应用：C++23 / `wasm32-wasip1`，
设备上跑 xtensa AOT。内部代码在 `pal` 命名空间，按 `pal_*`（数据与规则）+ `scenes/*`（画面）分层。

这份文档只记录**改完怎么验证**和**验证时容易踩的坑**。改动前先读第 1 节和第 7 节。

---

## 1. 改完代码的标准流程

```powershell
# 1) 编译（-Wall -Wextra -Werror，未使用的参数/变量会直接编译失败）
micropixel build

# 2) 静态校验 + 平衡模拟（约 10 秒，300 次通关模拟）
powershell -NoProfile -File tools\run-battle-sim.ps1 -Runs 300

# 3) 上设备（会重新打包 xtensa AOT、安装并启动）
micropixel --port COM3 run --aot-target xtensa --no-follow
```

- `micropixel` 在 `C:\Users\gil\AppData\Local\MicroPixel\bin\micropixel.exe`；
  下面示例里的 `micropixel` 都代指这个完整路径（它不一定在 PATH 里）。
- `tools/run-battle-sim.ps1` 用 SDK 自带的 wasi-sdk 编译
  `tools/battle_sim.cpp` + `pal_battle/pal_model/pal_content`，再用 node 的 WASI 跑。
  **前置条件**：先 `micropixel build` 过一次，否则没有 `build/generated/pal_strings.hpp`。
- 脚本先做静态校验（失败会打印 `PROBLEM:` 并以退出码 1 结束）：
  - 每张地图从出生点洪泛填充，检查出口、出生点、每个 NPC 是否可达（NPC 四面至少一格可达）
  - 对话/战斗/脚本步骤的所有 id 是否越界
  - 存档往返 + v1/v2 旧存档的索引迁移（用例表在 `tools/battle_sim.cpp` 里，插章节时补一行）
- 然后跑三种打法的完整通关模拟，看胜率与平均回合数：
  - `smart`、`skills-no-items` 必须**全部**通关（默认 300 局）；
  - `attack-only` 在需要技能的战斗上失败是**预期**的（不要为了让它通过去削数值）。
- 只改文案或立绘时，第 2 步仍然要跑：它同时负责内容校验。

## 2. 设备操作要点

设备：SenseCAP Watcher（ESP32-S3，面板 412×412，固件 0.9.5），底部 USB-C 的 CH342 桥。

- **每条命令都要带 `--port COM3`**：`micropixel port list` 返回 0 个端口（只接受 VID 0x303A）。
- 没有 `app status`；用 `micropixel --port COM3 device status`。
- `app last-error` 看崩溃原因；`guest_trap: unreachable` 这类信息只在这里，截图看不出来。
- `logs -n N --source app|host|all` **永远不会自己返回**：不要同步执行，包在后台 job 里加超时，
  或放异步终端里跑完就 kill。
- 输入：`input tap X Y`、`input press up|down|left|right|confirm|back|south|menu`、
  `input swipe X1 Y1 X2 Y2 --duration-ms 300 --steps 8`（拖拽走路用这个）。
- `input tap X Y --screenshot F` 抓的是**点下去之前**那一帧；要看点击结果就分开两条命令。
- 每条命令一个来回大约 0.2–1 秒，所以 1.5 秒的提示条（"已记录天机"）经常在截图前就消失了：
  **用状态变化验证行为**（道具数量、血量、场景），别指望截到提示条。
- 近似帧不要用眼睛判断有没有变化，逐像素比；曾两次"看出"移动，其实是同一帧。

## 3. 坐标（412×412，safe_area {60,60,292,292}）

面板是直径 412 的**圆**，`safe_area` 正是它的内接正方形（292 ≈ 412/√2，四周各 60）。
正方形到圆周之间那一圈（本文叫“环带”）同样属于 App Surface，而且**可以点**——2026-09 在设备上验证过：
点 (206,30) 走了一格并触发上方 NPC 对话。所以方向键搬到了环带的四个边中点，正方形整块留给画面。

设备上点击坐标就是面板像素，不需要加安全区偏移。已量准的常用点：

| 位置 | 坐标 |
| --- | --- |
| 标题：新的旅程 / 继续前缘 | (206, 223) / (206, 263) |
| 探索：方向键 上 / 左 / 右 / 下（环带按键 97×40） | (206, 30) / (30, 206) / (382, 206) / (206, 382) |
| 探索：状态按钮（正方形内右下角，避开下键） | (315, 330) |
| 战斗：指令面板 | y 274..352（消息条 y 251..274） |
| 战斗：指令按钮（3 列 × 2 行） | 列 x 63..156 / 159..252 / 255..348；行 y 277..311 / 314..348 |
| 战斗：术法/道具列表（2 列） | 列 x 64..204 / 206..346；行 y 278..311 / 313..346 |
| 状态：行囊第 1 / 2 行 | y 234..270 / 271..306 |

**布局改了就必须重新算**：这些来自 `pal_common.cpp` 的 `BuildLayout`（环带几何在 `MeasureRing`）
和各场景的 rect 辅助函数（`CardRect`、`BagRect`、`ListRowRect`、`EnemyRect`、`PartyRect`），
不要凭截图目测。

## 4. 脚本化操作的两个坑

- PowerShell 的辅助函数（`$global:m`、`tap`、`shotnow` 等）**只存在于当前终端会话**，
  终端一被杀就没了。所以要么每次在同一条命令里先定义，要么直接用完整路径。
- 单个批次别太长（60 次按键以内）：长循环被打断或同终端再发命令会被杀掉，
  跑长流程时每批之间看一次截图确认位置。

```powershell
$m='C:\Users\gil\AppData\Local\MicroPixel\bin\micropixel.exe'
1..30 | ForEach-Object { & $m --port COM3 input press confirm 2>&1 | Out-Null; Start-Sleep -Milliseconds 400 }
& $m --port COM3 screenshot --output "$env:TEMP\shot.jpg" | Out-Null
```

## 5. 手动通关检查清单

- 标题：有存档时"继续前缘"可点；新游戏从第 0 步开始
- 对话：打字机、长句自动换行（不再溢出对话框）、旁白无立绘、说话人名牌、三选项列表可选
- 地图：方向键走格、拖拽/滑动走多格、出口闪烁、走到 NPC 面朝方向触发对话、返回地图继续
- 战斗：攻击 / 术法 / 道具 / 防御 / 遁走；点敌人选目标；瞄准时点指令区可取消；胜利结算显示修为灵石；
  全灭后自动重打同一场
- 状态：存档、点行囊用药（满血/满蓝时不消耗）、返回原位
- 章节过渡：对话 → 地图 → 战斗 → 下一段对话

## 6. 内容改动规则

- 文案只放 `i18n/zh-CN.json`。key 规则 `[a-z][a-z0-9]*(?:[._-][a-z0-9]+)*`，
  生成枚举是 `k` + PascalCase，落在 `build/generated/pal_strings.hpp`；
  **改完 i18n 必须重新 `micropixel build`** 才有新枚举。
- 加内容 = 加 i18n key + 在 `pal_content.cpp` 的表里加行。`kDialogue[]` **按位置索引**，
  中间插入节点时必须同时插入数组行（表尾有 `static_assert` 兜长度，但掩护不了错位）。
- 固定上限（超了会编译期或运行期出问题）：地图 NPC 4、地图触发器 2、每战敌人 3、
  每敌技能 2、每角色技能 4、队伍 3、行囊 8 格。行囊里有 8 种道具就再也装不进新道具。
- 往剧情中间插步骤：升 `kSaveVersion`、在 `ProgressDeserialize` 里补旧存档索引换算，
  并在 `tools/battle_sim.cpp` 的迁移用例表里加一行。
- `app_id` 最后一段决定产物名（`com.maker.pal` → `pal_strings.hpp` / `build/pal.aot`），
  改 `app_id` 要同步 `#include` 和 `namespace ids =`。
- 新敌人/新立绘走 `widgets::EnemySprite` / `widgets::Portrait` 的 selector，
  `CharacterDef::portrait` 和 `MapNpcDef::portrait` 用的是同一套编号。

## 7. 已知坑（都真踩过）

- **别每帧无条件重绘**。曾经每个 tick 都整屏重绘，一次提交 64 ms，Host 不停拒绝
  （`present rejected`），状态菜单点了没反应，最后应用 `guest_trap`。
  现在渲染按 `context.dirty` 走，`GameView` 只发送**变化过**的节点属性；
  新场景请照这个模式写（战斗只在状态或伤害数字变化时刷新，对话只跟打字机）。
- **节点创建会失败**，失败就丢这一帧，不要 `.value()`（会直接崩）。
- 战斗瞄准状态指令面板是空的（技能与道具列表在那一刻没有），所以现在会画一行
  "选择敌人 + 目标名"；新增战斗状态时注意别让玩家面对空面板。
- 锁妖塔是唯一的迷宫地图，直着往下会被墙挡住。从出生点 (7,4) 到出口的路线：
  左、下、下、左、下、下、右、下、下、右。
- **方向键的位置取决于面板形状**：`MeasureRing()` 量出环带够深（≥ 34 px）时才把四个键放到环带并
  把 footer 压到 h 的 15%（正方形几乎全给画面）；长方形宿主机（`safe_area` == surface，没有环带）
  自动退回 footer 里的十字键 + 54% footer。动 footer 比例或 dpad 时两条路径都要过一遍。
- 临时调试块（`scenes/title_scene.cpp` 里的 `DEBUG-JUMP` / `DEBUG-XP`）方便跑通后半段剧情，
  但**提交前必须删掉**（没有它们时标题的"新的旅程"从第 0 步开始）。
- 提交信息用英文；`build/`、`screenshots/`、`devlog.txt`、`.env`、`micropixel.lock.json`
  已经在 `.gitignore` 里，不要强加。

## 8. 还没做的事

- 技能按等级解锁：现在 1 级就开放全部四个技能（含终极技）
- 火灵符 / 蛮牛角 / 灵石（金币）没有获得与消耗途径，没有商店
- 每次进入新地图都会全员回满血，道具与灵力管理基本失去意义
- 白河村 → 苗疆 → 毒瘴谷 → 神木林 → 女娲神殿 → 南诏祭坛 → 结局这一段
  只过了脚本校验，没在设备上完整走过
- 蛛后偏弱（模拟胜率 100%、平均 9 回合），最终战约 72–88%、平均 14 回合
