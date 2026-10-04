# 上手：第一次拿到这个仓库

目标：**从零到"看到东西在跑"，并且知道下一步该看哪份文档。** 约 10 分钟（不含下载 SDK）。

## 0. 你需要什么

| 你有 | 能做到 |
| --- | --- |
| 只有一台电脑 | 构建 + 跑完整通关模拟（校验 67 步剧本 / 15 张地图 / 17 场战斗全部正常） |
| 再加一台 MicroPixel 设备 | 真机运行、截图，改完立刻上机看效果 |

**没有设备也能验证这个仓库是好的**（第 3 步），只是看不到画面——本仓库没有桌面模拟器。

## 1. 装 CLI（一次性）

`micropixel` 是官方 CLI，构建、打包、烧录、截图、发输入都靠它。

1. 按官方文档安装 SDK：https://micropixel.ai/docs/environment/
2. 想让 AI 帮你装：https://micropixel.ai/docs/ai-setup/
3. 装完**新开一个终端**，确认：

```powershell
micropixel --version
```

装不上的常见原因见官方 FAQ：https://micropixel.ai/docs/environment-faq/

> 本仓库记录的基线是 SDK 0.20.1，官方当前发布 0.20.2。升级前先读
> https://micropixel.ai/docs/ai/content/reference.md 的兼容性说明。

## 2. 打开项目

```powershell
git clone https://github.com/maker-community/micropixel-workshop.git
cd micropixel-workshop
code .
```

**用 VS Code 打开仓库根目录**，不要只打开 `apps/pal`：根目录才是这个"合集"，
仓库级规则（根 `AGENTS.md`）和技能（`.github/skills/`）都在这一层，AI 才吃得到。

但**命令行要进到 app 目录里执行**——CLI 以 `app.json` 所在目录为准：

```powershell
cd apps/pal
```

## 3. 先做"不需要设备"的验证

```powershell
cd apps/pal
micropixel build
powershell -NoProfile -File tools\run-battle-sim.ps1 -Runs 300
```

第一条编译（约 10 秒）。第二条是**无头模拟器**：把整条剧本用三种打法各从头玩 300 遍，
同时做内容校验（地图可达性、id 越界、存档往返）。

**看到下面这些才算成功：**

```
content validation: 0 problem(s), 67 script steps, 15 maps, 42 dialogue nodes
== policy smart: 300/300 runs finished, ... battle retries total
== policy skills-no-items: 300/300 runs finished, ...
== policy attack-only: 0/300 runs finished, ...
```

- `smart` 与 `skills-no-items` **必须 300/300**——这是本仓库的硬门禁
- `attack-only` **一定失败**，这是设计如此：有些战斗必须用技能，
  不要为了让它通过去改数值

跑不通先看第 6 节。

## 4. 有设备：跑在真机上

1. **找到你的串口号**。示例里的 `COM3` 是开发机上 CH342 桥的**控制口**，你的机器很可能是别的号。
   在 Windows 设备管理器的“端口 (COM 和 LPT)”里看：CH342 会暴露**两个**口（控制 + 固件），
   选靠前的那个控制口。
   两个陷阱：`micropixel --transport usb device status` 在 CH342 这种桥上会报
   `No MicroPixel USB device found`；`micropixel port list` 也会返回 **0 个端口**
   （它只认 ESP32 原生的 USB-JTAG VID）。**不要据此判断设备没连上**，直接用
   `micropixel --port COM<n> device status` 逐个试即可。
2. 装机并启动（把 `COM3` 换成你的端口）：

```powershell
micropixel --port COM3 run --aot-target xtensa --no-follow
```

`--no-follow` = 装完启动就返回。不加它会跟随日志，看起来像"卡住"（见第 6 节）。

3. 截图确认（截图默认被 gitignore，不会污染仓库）：

```powershell
micropixel --port COM3 screenshot --output shot.png
```

4. 你应该看到 pal 的标题画面（"新的旅程 / 继续前缘"）。选"新的旅程"开始游戏。

**设备的操作方式**：环带上的四个方向键走路/选择；确认键或触屏推进对话与确认；
右下角的**状态按钮**开菜单（队伍 / 行囊 / 存档）。战斗里用触屏点指令按钮最稳。

## 5. 让 AI 接手

规则和技能都在版本库里，克隆下来就能用，**不需要额外配置**：

| 你想做什么 | 对 AI 说 |
| --- | --- |
| 新加一个 app（游戏或小工具） | `/micropixel-new-app` |
| 上机运行、验证、排查冻屏/黑屏 | `/micropixel-run-on-device` |
| 发布到商店、准备截图与说明 | `/micropixel-publish` |

AI 会自动读这些文件，所以**你不用先解释项目背景**：

- 根 `AGENTS.md`：跨 app 规则——许可、提交、目录约定、设备要点
- `apps/<app>/AGENTS.md`：这个 app 的验证流程与坑（pal 的坐标表、缺字冻屏等）
- `.github/skills/`：按需加载的工作流（即上面三条斜杠命令）

第一句话可以就照抄："先读 AGENTS.md 和相关 skill，然后帮我做 X。"

## 6. 第一次就会遇到的报错

| 现象 | 原因与处理 |
| --- | --- |
| `micropixel` 不是可识别的命令 | CLI 没装或没进 PATH：装完 SDK 后**新开终端**；装的时候勾选加入 PATH |
| 报找不到 `app.json` | 你在错误的目录：先 `cd apps/<app>` 再执行 |
| `Cannot open USB device COMx` | 端口号不对（换成你的）、设备没插，或上一个 `logs`/`run` 还占着串口。先翻一下第 4 章第 1 条：`port list` 返回 0 不代表设备不在 |
| `micropixel logs` 一直不返回 | 它默认跟随日志，**永远不会自己结束**：`Ctrl-C` 只停日志，或改用 `run --no-follow` |
| `guest_trap: unreachable` | 客机里抛异常了：`micropixel --port COM3 app last-error` 看原因 |
| 屏幕卡死不动 | 先别下结论：可能是"缺字冻屏"（逻辑其实还在跑），见 `apps/pal/AGENTS.md` 第 7 节 |
| 平衡门禁没过（`smart` 不是 300/300） | 你刚改过数值/技能/抗性？看 `apps/pal/AGENTS.md` 第 1 节的门禁规则 |

## 7. 下一步看哪里

- 每个 app 是什么、怎么玩、怎么改：`apps/<app>/README.md`
- 改代码前必读：`apps/<app>/AGENTS.md`
- 平台能力与限制（机器可读，AI 也会读）：https://micropixel.ai/docs/ai/index.json
- 许可与内容声明：`LICENSE` / `NOTICE`
