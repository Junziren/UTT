# UTT - Utility Toolbox

**VST3 通道条工具插件** | Developer: **UnpureBloom**  
Bilibili: [space.bilibili.com/227573145](https://space.bilibili.com/227573145)

---

## 功能概述

UTT 是一款综合性的立体声通道条工具，适用于混音、母带处理和音频分析。
v0.3.0 完成 DSP 正确性重写（BS.1770 响度、实时安全录音、参数平滑），界面更换为
基于 WebView2 的纸面仪器风格 UI。

### DSP 处理模块

| 模块 | 功能 |
|------|------|
| **Gain** | -96 dB 至 +24 dB 增益控制（10ms 平滑，无 zipper noise） |
| **Pan** | 立体声左右声像定位（中点连续的定功率法则） |
| **Phase** | L/R 独立相位反转 + 0-360° 相位偏移（级联全通、采样率补偿、0° 严格旁路） |
| **MS Mode** | Mid/Side 编码/解码 |
| **Mono** | L+R 合并 / Mid Only / Side Only |
| **Stereo Separation** | 立体声分离度 0-100% |
| **Mono to Stereo** | 单声道复制到双声道 |
| **Channel Swap** | 左右声道交换 |
| **DC Offset** | 直流偏移消除 |
| **Output Stage** | L/R 独立增益 + Mute + Solo |

### 测量与可视化（真实遥测）

| 功能 | 说明 |
|------|------|
| **电平表** | L/R 峰值 + RMS，峰值保持 |
| **相位相关系数** | -1 (反相) 至 +1 (同相) 实时显示 |
| **示波器** | L/R 波形（三缓冲快照，无撕裂） |
| **LUFS 响度** | ITU-R BS.1770-4 / EBU R128：Momentary / Short-term / Integrated（含门限） |

### 录制

- 后台 writer 线程 + 无锁 FIFO，音频线程零磁盘 I/O
- 点击 **Rec** 立即开始录制；未选择自定义路径时，会在 `Music/UTT Recordings` 创建唯一的 WAV 文件，再次点击停止
- 点击 **浏览** 可选择自定义保存路径和文件名；录制中更换路径会自动结束当前文件并开始新的 take
- 输出格式：24-bit WAV，跟随宿主采样率

### 界面

- **WebView2 离线内嵌 UI**（纸面仪器风格：纸色面板、方角硬边、深色显示井、机械开关/旋钮）
- 旋钮协议：拖拽 320px 行程（Shift 精细 1600px）、滚轮微调、双击复位、右键转宿主参数菜单、完整键盘操作
- 所有参数支持悬停与键盘聚焦说明，包含实际信号路径；M/S 明示 L/R 与 Mid/Side 的编码、解码公式
- IKB 参数反馈只在直接操作时短暂显示，支持 `prefers-reduced-motion`，不伪造 meter 或 scope 数据
- 未安装 WebView2 运行时时自动降级为极简原生面板（音频功能不受影响）

---

## 安装

### 从安装包 (推荐)

1. 运行 UTT_Setup.exe
2. 安装器会先处理 WebView2 Evergreen 离线运行库与 VC++ x64 运行库
3. VST3 插件将安装至 C:\Program Files\Common Files\VST3\
4. 重新启动 DAW 以识别新插件

### 手动安装

将 UTT.vst3 文件夹复制到：

```
C:\Program Files\Common Files\VST3\
```

### 支持的宿主

- Ableton Live 10+
- FL Studio 20+
- Cubase 10+
- Studio One 4+
- REAPER 6+
- Bitwig Studio 3+
- 其他支持 VST3 的宿主

---

## 构建 (开发者)

### 环境要求

- **CMake** >= 3.22
- **MSVC** (Windows) 或 **Xcode/Clang** (macOS)
- **JUCE** 8.0.13（已包含于仓库）
- Windows 构建 WebView UI 需要 WebView2 SDK（NuGet 包 `Microsoft.Web.WebView2`，
  位于 `%LOCALAPPDATA%\PackageManagement\NuGet\Packages`，JUCE 自动发现）

### Windows 构建

```powershell
git clone --recurse-submodules <repo-url>
cd UTT
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UTT_All UTTTests
```

### 单元测试

```powershell
cmake --build build --target UTT_All UTTTests
./build/UTTTests_artefacts/Release/UTTTests.exe
```

覆盖：LUFS -23 LUFS 校准信号、相位偏移 1kHz 验证、参数契约
（config/parameters-v2.json ↔ Parameters.h）、录音 FIFO、旧版状态迁移。

### Windows 发布包

```powershell
.\installer\build-installer.ps1 -DownloadPrerequisites
```

输出目录为 `dist\UTT-0.3.0-windows-x64\`，包含 VST3、Standalone、单文件
`UTT_Setup.exe` 与 `SHA256SUMS.txt`。安装器的前置运行库签名会在打包前校验。

### macOS / Apple 车道

```bash
cmake -B build-mac -G Xcode -DCMAKE_BUILD_TYPE=Release   # VST3 + AU + AUv3
cmake --build build-mac --config Release --target UTT_All UTTTests
```

AUv3/iOS 为**预备车道**，本仓库尚无 Apple 侧验证证据，
流程与验收清单见 [docs/APPLE_LANE.md](docs/APPLE_LANE.md)。

---

## 开源许可

UTT 基于 **GNU General Public License v3.0** 发布。

- 您可以自由使用、修改和分发本软件
- 修改后的版本必须同样以 GPLv3 开源
- 本软件不提供任何形式的担保

完整许可证文本见 [LICENSE](LICENSE) 文件。

*本插件使用 [JUCE](https://juce.com) 框架构建，JUCE 以 GPLv3 或商业许可方式提供。*

---

## 项目结构

```
UTT/
├── CMakeLists.txt          # 构建配置（格式矩阵：Win VST3+Standalone / Apple VST3+AU+AUv3）
├── config/
│   └── parameters-v2.json  # 参数契约（宿主 ABI 真源）
├── docs/
│   ├── IDENTIFIERS.md      # 标识符冻结合同（bundle id / FourCC / 参数 ID）
│   └── APPLE_LANE.md       # AUv3 预备车道与验收清单（UNVERIFIED）
├── resources/ui/           # WebView 前端（index.html / styles.css / app.js，离线打包）
├── src/
│   ├── PluginProcessor.cpp # DSP 链路与状态
│   ├── PluginEditor.cpp    # 编辑器宿主（WebView / 原生降级）
│   ├── Parameters.h        # 参数定义
│   ├── dsp/                # DSP 模块（PhaseProcessor / LUFSMeter / Oscilloscope …）
│   ├── gui/
│   │   ├── UTTWebView.cpp  # WebView 宿主 + 原生桥（合同 v1）
│   │   └── FallbackPanel.cpp # 无 WebView2 时的极简原生面板
│   ├── recorder/           # 后台线程录音（SPSC FIFO + writer 线程）
│   └── util/CrashLog.h     # 崩溃日志（写入 App Support，沙盒安全）
├── tests/                  # pluginval 配置与结果
└── legacy/                 # v0.2 原生 UI 与历史原型（已退役）
```

---

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| **v0.3.0** | 2026-08-22 | DSP 正确性重写（BS.1770 LUFS、录音实时安全、参数平滑、伪参数清理 v2）；WebView2 纸面仪器 UI、参数信号路径帮助与局部 IKB 反馈；离线运行库安装器；pluginval strictness 10 |
| v0.2.0 | 2026-06-04 | 录制功能完成，About 界面，品牌更名 UnpureBloom |
| v0.1.0 | 2026-05 | 初始版本，基础 DSP + GUI |

---

## 联系方式

- **Bilibili**: [UnpureBloom](https://space.bilibili.com/227573145)
- **Issues**: 请在项目仓库提交 Issue
