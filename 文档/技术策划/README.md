# 技术策划案

每个大系统一份。本文只讲**职责、功能、参数**。字段权威仍看 [数据](../数据/关卡数据.md)；系统清单看 [设计/系统](../设计/系统.md)。

```
人（3C）只发许可
        ↓ 方向 + bCanPush
格子模拟 改格子
        ↓ 问
地图管理器 这一格是什么
        ↓
交互物运行时 / 程序化地形 跟着播
```

| 文档 | 代码入口 |
|---|---|
| [地图管理器](地图管理器.md) | `UBoxBoard` |
| [格子模拟](格子模拟.md) | `UBoxGridSim` |
| [流程管理器](流程管理器.md) | `UBoxGameInstance` + `ABoxGameMode` |
| [交互物运行时](交互物运行时.md) | `UInteractableDef` + `ABoxInteractableActor` |
| [角色3C](角色3C.md) | `UPlayerDef` + `ABoxPlayerCharacter` |
| [程序化地形](程序化地形.md) | `ABoxMatchWorld` 刷 ISM |
| [表现](表现.md) | 插值 / 状态显隐 / 相机 |
| [事件](事件.md) | 模拟事件名；踏板 `EventId` |
| [存档](存档.md) | `UBoxSaveGame` |
| [UI](UI.md) | UMG，尚未做 |
| [校验](校验.md) | `ULevelData::Validate` 等 |
| [关卡编辑器](关卡编辑器.md) | 独立 Editor 模块，尚未做 |
