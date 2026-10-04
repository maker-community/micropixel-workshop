# docs/images

这个目录里的图片**会进版本库**（和 `screenshots/` 不同——那个目录在 `.gitignore` 里，
是设备抓图的临时存放处，用来验证行为可以，正文引用就会丢）。

## 命名约定

小写、模块名 + 画面名，同一种画面用连字符序号：

```
pal-title.png     标题画面（新的旅程 / 继续前缘）
pal-explore.png   探索画面（地图 + NPC + 环带方向键）
pal-battle.png    战斗画面（指令面板 + 敌人卡片 + 队伍血条）
```

## 拍这三张就够

在 app 目录里执行（`COM3` 换成你那台机器的端口，见 `docs/getting-started.md` 第 4 章）：

```powershell
cd apps/pal
micropixel --port COM3 screenshot --output ..\..\docs\images\pal-title.png
# 在设备上操作到下一个画面，再拍第二张
micropixel --port COM3 screenshot --output ..\..\docs\images\pal-explore.png
micropixel --port COM3 screenshot --output ..\..\docs\images\pal-battle.png
```

注意：

- **先停手再拍**：`screenshot` 抓的是"当前帧"，动画中的伤害数字、正在消失的提示条都会留在图里。
  等画面稳定 1-2 秒再截。
- 想确认拍到的确实是想要的画面，单独跑一次 `screenshot` 再重拍即可，不用重新烧录。
- **别用启动封面代替游戏截图**（封面是启动资产，发布商店时也不算作"实际运行画面"）。
- 每张控制在 ~500 KB 以内、PNG 或 JPEG，避免仓库膨胀。

## 加完之后

告诉 AI："把 `docs/images` 里的三张图嵌进 `docs/getting-started.md` 和 `apps/pal/README.md`
的对应位置"，它会放到该放的地方并对齐行文；也可以自己插，插图最适合的位置是：

| 图 | 放哪 |
| --- | --- |
| `pal-title.png` | `docs/getting-started.md` 第 4 章第 4 步（"你应该看到标题画面"） |
| `pal-explore.png` | `apps/pal/README.md` 的"怎么玩"——探索一条 |
| `pal-battle.png` | `apps/pal/README.md` 的"怎么玩"——战斗一条 |
