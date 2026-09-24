# UI · 技术策划案

游戏内菜单用 UMG。编辑器面板不是这套。  
产品级要能进能出，不是一张关卡直接开打。

代码：**尚未做。** 流程和存档 API 已在 GI / GM 上，UI 只消费。

---

## 职责

**管**

- 主菜单、选关、对局 HUD、暂停、胜利、设置

**不管**

- 格子规则
- 关卡编辑器 Slate
- 把开局 / 写档写进按钮蓝图里绕过流程管理器

---

## 功能

| 界面 | 内容 | 数据来源 |
|---|---|---|
| 主菜单 | 开始、设置、退出 | 开始 → 选关或直接 `PlayLevel` |
| 选关 | 官方顺序、未通关下一关锁定、星级 | `GetSelectEntries()` |
| 对局 HUD | 步数、暂停入口 | Sim `MoveCount` |
| 暂停 | 继续、重开、回选关 | GM `RequestPause` / `Restart` / `ReturnToSelect` |
| 胜利 | 再玩、下一关、回选关；显示星级 | `OnMatchWon`（LevelId, Moves, Stars） |
| 设置 | 主音量，立刻生效并写档 | `SetMasterVolume` |

试玩对局：UI 标「试玩」；代码已不写档。

---

## 参数

UI 本身几乎不配表。要绑的流程参数：

| 参数 | 在哪 | 说明 |
|---|---|---|
| `SelectMap` / `PlayMap` | GI | 选关图、对局图 |
| `FBoxSelectEntry.*` | GI 拼出 | 卡片显示 |
| `ParMoves` | 关卡 DA | 星级对照 |
| `MasterVolume` | 存档 | 0–1 |
| `bPlaytest` | 进关请求 | 隐藏或禁用写档相关文案 |

星级算法不在 UI 里重写，用 `BoxFlow::ComputeStars(Moves, ParMoves)`。
