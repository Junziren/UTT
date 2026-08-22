# UTT VST3 插件 — 开发计划 (v3)

> v3 取代 v2 计划（2026-08-22）。v2 描述的 SSL/API 主题系统与原生 JUCE GUI 已在 v0.3.0 退役，
> 相关实现归档于 `legacy/`。

## 摘要

UTT 是一款音频工具集合型 VST3 插件（JUCE 8.0.13 / C++17 / GPLv3），面向
House/Techno 制作与混音的立体声通道条工具。v0.3.0 完成 DSP 正确性重写与
WebView2 纸面仪器 UI，并为 Apple AUv3 车道完成代码与构建预备。

- 参数合同真源：`config/parameters-v2.json`（17+1 参数，ID 顺序为发布 ABI）
- 标识符冻结合同：`docs/IDENTIFIERS.md`
- Apple 车道（UNVERIFIED）：`docs/APPLE_LANE.md`

## 技术选型

| 项 | 方案 |
|---|---|
| 框架 | JUCE 8.0.13（仓库内 vendored） |
| 许可证 | GPLv3 |
| 构建 | CMake ≥3.22 + Ninja + MSVC 2022（需 vcvars64 环境） |
| GUI | WebView2 离线内嵌（`resources/ui/`，juce_add_binary_data 打包；无运行时时降级原生极简面板） |
| 插件格式 | Windows/Linux: VST3 + Standalone；macOS: VST3 + AU + AUv3；iOS: AUv3 + Standalone |
| 测试 | juce::UnitTest 控制台跑（`UTTTests`，12830 断言） |
| 验证 | pluginval strictness 10（系统安装副本） |

## 信号处理链路 (固定顺序)

```
Input L/R（入口 NaN/Inf 清洗；mono 输入复制到立体声 scratch，预分配分块处理）
  → [1] DC Offset Removal        (Bypass/On，一阶 HPF)
  → [2] Phase Invert             (L/R 独立 ×-1)
  → [3] Phase Offset             (0°~360°，两节级联一阶全通，1kHz 参考闭式解，
                                  系数随采样率补偿；0° 硬旁路 bit-exact)
  → [4] Gain                     (-96~+24dB，SmoothedValue ~10ms)
  → [5] MS Encode/Decode
  → [6] Mono Operations          (Off/L+R/Mid/Side)
  → [7] Stereo Separation        (0~100%，side 增益权重)
  → [8] Mono→Stereo Copy
  → [9] Channel Swap
  → [10] Pan                     (-100~100，中点连续定功率法则)
  → [11] Output Stage            (L/R Gain -96~+12dB + Mute + Solo，独立平滑)
  → Output L/R
```

实时安全：processBlock 零分配/零锁/零 I/O；录音为后台 writer 线程 + 预分配
SPSC FIFO（音频线程只入队）；示波器三缓冲发布。

## 测量（并行，不影响信号；原子量发布）

- Peak/RMS（L/R，采样率自适应平滑）
- 相位相关系数
- LUFS：ITU-R BS.1770-4 / EBU R128（K-weighting 按采样率闭式计算，
  M 400ms / S 3s / Integrated 带门限，-23 LUFS 校准测试覆盖）
- 示波器：maxPoints 2048 三缓冲，UI 侧抽稀到 256 点 @12Hz

## UI（FAD Paper Instrument 设计语言适配）

纸色面板 #E8E5DB / chrome #F6F4EC / ink #1C1B14，方角硬边、深色显示井、
I/0 机械开关、旋钮完整协议（拖拽 320px、Shift 1600px、滚轮 0.01/0.001、
双击复位、右键宿主菜单、键盘）。原生桥合同 v1 见 `src/gui/UTTWebView.h` 注释：
值/显示文本/默认值全部 native-authoritative；浏览器预览模拟数据与桥严格隔离。

## v0.3.0 完成状态（2026-08-22）

| 项 | 状态 | 证据 |
|---|---|---|
| Phase A DSP 重写（A1–A9） | ✅ | commit bc5fdf4；12830 单测绿 |
| Phase B WebView UI | ✅ | commit d49523f；4 视口视觉 QA + Standalone 原生桥冒烟 |
| Phase C AUv3 预备 | ✅ | commit dd2e8fa；CMake 条件格式 + 沙盒整改 + 两份合同文档 |
| D3 pluginval strictness 10 | ✅ | 4 次运行 exit=0（含 VST3 validator、自动化测试） |
| D4/D5 清理 + 0.3.0 + 文档 | ✅ | commit dd2e8fa；-95MB 垃圾、build/ 解除跟踪 |

## 明确未完成 / 未验证（后续计划）

| 项 | 层级 | 说明 |
|---|---|---|
| DAW 实机手测 | DAW-tested | Live/REAPER 中开编辑器、跑自动化、长时稳定性；本机无 DAW 自动化路径 |
| 安装器更新 | release-ready | installer/ 仍是 v0.2；需内嵌 WebView2 Evergreen Offline x64 + VC++ runtime，断网 VM 冷装验证 |
| Apple 全部项 | device-tested | 见 docs/APPLE_LANE.md §4 清单（auval、AUv3 注册、宿主矩阵、内存警告） |
| 发布产物 | release-ready | 签名、notarization、哈希、Release Notes |

## 构建速查

```powershell
# 需要 MSVC 环境（vcvars64）与 WebView2 NuGet 包（JUCE 自动发现）
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UTT        # VST3 + Standalone
cmake --build build --target UTTTests   # 单元测试
./build/UTTTests_artefacts/Release/UTTTests.exe
# 安装副本验证
./tools/pluginval/pluginval.exe --validate "C:\Program Files\Common Files\VST3\UTT.vst3" --strictness 10
```
