# UTT - Utility Toolbox

**VST3 通道条工具插件** | Developer: **UnpureBloom**  
Bilibili: [space.bilibili.com/227573145](https://space.bilibili.com/227573145)

---

## 功能概述

UTT 是一款综合性的立体声通道条工具，适用于混音、母带处理和音频分析。

### DSP 处理模块

| 模块 | 功能 |
|------|------|
| **Gain** | -96 dB 至 +24 dB 增益控制 |
| **Pan** | 立体声左右声像定位 |
| **Phase** | L/R 独立相位反转 + 0-360 度相位偏移 |
| **MS Mode** | Mid/Side 编码/解码 |
| **Mono** | L+R 合并 / Mid Only / Side Only |
| **Stereo Separation** | 立体声分离度 0-100% |
| **Mono to Stereo** | 单声道复制到双声道 |
| **Channel Swap** | 左右声道交换 |
| **DC Offset** | 直流偏移消除 |
| **Output Stage** | L/R 独立增益 + Mute + Solo |

### 测量与可视化

| 功能 | 说明 |
|------|------|
| **VU 电平表** | L/R 双声道实时峰值 + RMS |
| **相位相关系数** | -1 (反相) 至 +1 (同相) 实时显示 |
| **矢量示波器/波形** | 立体声矢量图或 L/R 波形叠加 |
| **LUFS 响度测量** | Momentary / Short-term / Integrated |

### 录制

- 点击 **Rec** 启用录制，再次点击停止
- 点击 **Browse** 选择保存路径和文件名
- 输出格式：24-bit WAV，跟随宿主采样率

### 主题系统

- **Channel Strip** — 经典通道条风格
- **Dark Modern** — 深色现代风格
- **Retro Light** — 复古浅色风格

---

## 安装

### 从安装包 (推荐)

1. 运行 UTT_Setup.exe
2. 按照向导完成安装
3. VST3 插件将安装至 C:\Program Files\Common Files\VST3\
4. 重新启动 DAW 以识别新插件

### 手动安装

将 UTT.vst3 文件夹复制到：

`
C:\Program Files\Common Files\VST3\
`

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
- **MSVC** (Windows) 或 **Clang** (macOS/Linux)
- **JUCE** 7.x (已作为子模块包含)

### Windows 构建

`powershell
git clone --recurse-submodules <repo-url>
cd UTT
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cd build
ninja
`

### macOS 构建

`ash
git clone --recurse-submodules <repo-url>
cd UTT
cmake -B build -G Xcode
cmake --build build --config Release
`

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

`
UTT/
├── CMakeLists.txt          # 构建配置
├── LICENSE                 # GPLv3 许可证
├── README.md               # 本文件
├── src/
│   ├── PluginProcessor.cpp # DSP 处理核心
│   ├── PluginEditor.cpp    # GUI 编辑器
│   ├── Parameters.h        # 参数定义
│   ├── dsp/                # DSP 模块
│   │   ├── DCOffset.cpp
│   │   ├── GainStage.cpp
│   │   ├── MSProcessor.cpp
│   │   ├── ChannelProcessor.cpp
│   │   ├── Panner.cpp
│   │   ├── PhaseProcessor.cpp
│   │   ├── Metering.cpp
│   │   ├── PhaseMeter.cpp
│   │   ├── Oscilloscope.cpp
│   │   └── LUFSMeter.cpp
│   ├── gui/                # GUI 组件
│   │   ├── controls/       # VUKnob, VUMeter, LedButton
│   │   ├── theme/          # UTTLookAndFeel, ThemeLoader
│   │   └── AboutComponent.cpp # About 界面
│   ├── recorder/           # 音频录制
│   │   └── AudioRecorder.cpp
│   └── util/               # 工具
│       ├── CrashLog.h      # 崩溃日志
│       └── TraceLog.h      # 执行追踪
└── build/                  # 构建输出
`

---

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| **v0.2.0** | 2026-06-04 | 录制功能完成，About 界面，品牌更名 UnpureBloom |
| v0.1.0 | 2026-05 | 初始版本，基础 DSP + GUI |

---

## 联系方式

- **Bilibili**: [UnpureBloom](https://space.bilibili.com/227573145)
- **Issues**: 请在项目仓库提交 Issue
