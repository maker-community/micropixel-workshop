# pal — 回合制仙侠 RPG

`com.maker.pal`，MicroPixel 设备的访客应用。原创重制，致敬国产单机时代的仙侠题材。

15 张地图、17 场战斗、67 步剧本，从序章一路打到南诏祭坛。含五灵相克、按等级解锁的技能、
多结局判定、存档（v3，带 v1/v2 迁移）以及一个**无头平衡模拟器**。

## 先跑起来

```powershell
cd apps/pal
micropixel build                                                  # 编译，约 10 秒
powershell -NoProfile -File tools\run-battle-sim.ps1 -Runs 300    # 内容校验 + 平衡门禁
micropixel --port COM3 run --aot-target xtensa --no-follow        # 上设备（端口换成你的）
```

- 前两条**不需要设备**。门禁要求 `smart` 与 `skills-no-items` 各 **300/300** 通关，
  `attack-only` 失败是预期的。
- CLI 必须在**本目录**执行（以这里的 `app.json` 为准）；在仓库根目录跑会报找不到 `app.json`。
- 第一次上手、报错排查见 [`../../docs/getting-started.md`](../../docs/getting-started.md)。

## 怎么玩

- **标题**：选"新的旅程"开新档，"继续前缘"读存档
- **探索**：环带方向键走路（也支持拖动地图 / 点触）；走到地图出口进入下一段
- **对话**：确认键或触屏推进；出现选项时用方向键选择
- **战斗**：触屏点指令按钮最稳（3 列 × 2 行：攻击 / 术法 / 道具 / 防御 / 遁走）。
  boss 战里"遁走"会置灰（不能逃）；**全灭会自动重打同一场，不丢进度**
- **菜单**：右下角"状态"按钮 —— 队伍 / 行囊 / 存档

难度上有两个关键机制：**五灵相克**（打错属性伤害明显更低）和**技能按等级解锁**
（前期只有两个技能，终极技要到中后期）。水月宫的拜月教主是前期最紧的一场：
先用金刚咒减伤，再用观音咒群疗，别硬拼输出。

## 想改内容

内容基本数据驱动，多数改动只落在一个文件：

| 想改什么 | 改哪里 |
| --- | --- |
| 地图布局与 NPC | `pal_content.cpp` 的 `k*Rows`（地形）与 `kMaps` |
| 敌人 / 技能 / 物品 / 元素相克 | `pal_content.cpp`：`kEnemies`、`kSkills`、`kItems`、`kEnemyResist`、`kCharacterResist` |
| 剧本顺序（对话 / 探索 / 战斗） | `pal_content.cpp` 的 `kScript` |
| 对白文案 | `i18n/zh-CN.json` |
| 音效与 BGM | `audio/sfx.json`（ToneSpec 配方，不含音频资源） |
| 启动封面 | 改 `tools/make-launch-cover.ps1` 后重新生成，别手改那张 PNG |
| 界面布局与坐标 | `pal_common.cpp` 的 `BuildLayout` / `MeasureRing` |

改完**必须**跑一次 `tools\run-battle-sim.ps1`：它同时负责内容校验（地图可达性、id 越界、
存档往返）和平衡门禁。详细规则和坑见 [`AGENTS.md`](AGENTS.md)。

## 代码结构

| 文件 | 职责 |
| --- | --- |
| `main.cpp` / `pal_app.cpp` | 入口、场景栈、主循环、存档落盘 |
| `pal_content.*` | 全部内容表：地图 / 敌人 / 技能 / 物品 / 剧本 / 对话节点 / 元素与解锁等级 |
| `pal_model.*` | 进度、队伍、行囊、经验与升级 |
| `pal_battle.*` | 战斗规则：行动序、伤害与元素、逃跑判定、战斗后恢复 |
| `pal_world.*` | 地图、寻路、触发器 |
| `pal_dialogue.*` | 对话图与分支 |
| `pal_audio.*` | 音效 / BGM 调度（无音频资源，全部运行期合成） |
| `pal_common.*` / `pal_widgets.*` | 布局计算与通用绘制 |
| `scenes/*` | 六个场景：标题 / 探索 / 对话 / 战斗 / 菜单 / 结局 |
| `tools/` | 无头模拟器、设备验证脚本、封面生成 |

## 许可与内容

- 代码 MIT；第三方与内容声明见 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) 与仓库根 `NOTICE`。
- 非官方同人作品：所有美术由脚本生成、音频由运行期合成、文案为本项目原创，未包含任何商业作品素材。
