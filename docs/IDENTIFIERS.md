# UTT 标识符合同（Identifiers Contract）

> 对应 openFAD M0_CONTRACT 模式。**发布任何一个二进制之后，本文件中的所有标识符即刻冻结，不得更改。**
> 修改任何一个标识符 = 破毁宿主工程兼容性（DAW 工程按 ID 引用参数，macOS 按 bundle id/FourCC 识别插件）。

## 插件标识符

| 项 | 值 | 状态 |
|---|---|---|
| 产品名 | UTT | 冻结 |
| 制造商 | UnpureBloom | 冻结 |
| VST3 Manufacturer Code | `UT01` | 冻结 |
| VST3 Plugin Code | `UT02` | 冻结 |
| VST3 类 ID | 由 UT01/UT02 派生（f361fbd4-ee1c38c0 一族） | 冻结 |
| Bundle ID（macOS/iOS 应用族） | `com.unpurebloom.utt` | 冻结 |
| Bundle ID（AUv3 扩展） | `com.unpurebloom.utt.auv3` | 计划（首次发布前可调） |
| Bundle ID（iOS 容器 App） | `com.unpurebloom.utt.app` | 计划（首次发布前可调） |
| AU 类型 | `aufx`（effect） | 冻结 |
| AUv3 Main Type | `aufx` | 冻结 |

## 参数 ID（ABI，17 个）

顺序即宿主中的注册顺序。真源：`config/parameters-v2.json`（契约测试断言与 `src/Parameters.h` 一致）。

```
1  dcOffsetBypass      (bool)
2  phaseInvertL        (bool)
3  phaseInvertR        (bool)
4  phaseOffset         (float 0–360 deg)
5  gain                (float -96..24 dB)
6  msMode              (choice: Bypass/Encode/Decode)
7  monoMode            (choice: Off/L+R Sum/Mid Only/Side Only)
8  stereoSeparation    (float 0–100 %)
9  monoToStereo        (choice: Off/L->Both/R->Both)
10 channelSwap         (bool)
11 pan                 (float -100..100 %)
12 outputLGain         (float -96..12 dB)
13 outputRGain         (float -96..12 dB)
14 outputLMute         (bool)
15 outputRMute         (bool)
16 outputLSolo         (bool)
17 outputRSolo         (bool)
+ recordEnable        (bool)   ← 录制开关
```

规则：
- 禁止重命名、重排、重编号任何 ID；显示名（name）可以改。
- 新参数只能**追加**到列表末尾，并跑「旧 state 恢复」测试。
- 枚举 choice 的现有选项顺序冻结；新增选项只能追加，默认值索引不得移动。
- v0.2 的 9 个 meter 伪参数与 themeIndex 已在 v2 移除，恢复旧工程时静默忽略（有迁移测试覆盖）。

## 状态树属性（非宿主参数）

| 属性 | 类型 | 说明 |
|---|---|---|
| `recordPath` | string | 录制文件路径（UI/宿主状态） |
| `uiTheme` | int | 保留字段（v0.3 固定 0，兼容旧工程） |
| `customThemePath` | string | 保留字段（v0.3 不再使用，兼容旧工程） |

## 变更流程

任何想改标识符的 PR 必须：
1. 说明为什么兼容性破坏不可避免；
2. 更新本文件 + `config/parameters-v2.json` + 迁移测试；
3. 在 Release Notes 中标注 breaking change 与最低兼容版本。
