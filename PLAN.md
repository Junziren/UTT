# UTT VST3 插件 — 开发计划 (v2)

## 摘要

UTT 是一款音频工具集合型 VST3 插件，采用 JUCE 框架 (GPLv3)，默认主题为模拟 SSL/API 风格通道条硬件。信号链路固定顺序，每个模块可独立旁通。L/R 声道在输出级各有独立 Mute/Solo 和增益控制。内置 3 套可切换主题，同时支持外部 JSON 主题文件加载。

## 技术选型

| 项 | 方案 |
|---|---|
| 框架 | JUCE 8.x (Git submodule) |
| 许可证 | GPLv3 |
| 构建 | CMake 4.0 + MSVC 2022 (VsDevCmd.bat) |
| GUI | JUCE 原生 Component + 自定义 LookAndFeel |
| 最低 C++ | C++17 |
| 插件格式 | VST3 (JUCE 自动生成) |

## 信号处理链路 (固定顺序)

```
Input L/R
  → [1] DC Offset Removal        (Bypass/On, 一阶HPF @5Hz)
  → [2] Phase Invert             (L/R 独立开关 ×-1)
  → [3] Phase Offset             (0°~360°, 一阶全通滤波器)
  → [4] Gain (Linked)            (-∞ ~ +24dB, L/R 联动)
  → [5] MS Encode/Decode         (Bypass/Encode/Decode)
  → [6] Mono Operations          (Off/L+R Sum/Mid Only/Side Only)
  → [7] Stereo Separation        (0%~100%, M/S权重调节)
  → [8] Mono→Stereo Copy         (Off/L→Both/R→Both)
  → [9] Channel Swap             (Off/On, 交换L/R)
  → [10] Pan                     (-100~+100, 正弦/余弦法则)
  → [11] Output Stage
       ├─ L Gain (-∞~+12dB) + Mute + Solo
       └─ R Gain (-∞~+12dB) + Mute + Solo
  → Output L/R
```

## 测量模块 (并行，不影响信号)

| 模块 | 输出参数 | 实现 |
|---|---|---|
| Peak/RMS Meter | L Peak, R Peak, L RMS, R RMS (dBFS) | 平滑包络跟随器 |
| Phase Correlation Meter | -1.0 ~ +1.0 | 实时相关系数 |
| Oscilloscope | 环形缓冲区 (N samples) | 固定长度环形缓冲 |
| LUFS Meter | Momentary / Short-term / Integrated | EBU R128, K-weighting |

## 录制模块

- Record Enable 开关 + WAV 文件路径参数
- PCM WAV 格式，跟随 DAW 采样率/位深度
- 录制后处理级信号（含 Output Stage）

---

## 文件结构

```
D:\UTT\
├── CMakeLists.txt
├── juce/                             # JUCE git submodule
├── src/
│   ├── PluginProcessor.h/cpp         # AudioProcessor (DSP核心)
│   ├── PluginEditor.h/cpp            # AudioProcessorEditor (GUI根)
│   ├── Parameters.h                  # 全部参数ID/范围定义
│   ├── dsp/
│   │   ├── DCOffset.h/cpp            # DC偏移移除
│   │   ├── PhaseProcessor.h/cpp      # 相位反转+偏移
│   │   ├── GainStage.h/cpp           # 增益(含Output Stage)
│   │   ├── MSProcessor.h/cpp         # MS编解码
│   │   ├── ChannelProcessor.h/cpp    # Mono/Swap/Separation/Copy
│   │   ├── Panner.h/cpp              # 声像
│   │   ├── Metering.h/cpp            # Peak/RMS电平表
│   │   ├── PhaseMeter.h/cpp          # 相位相关表
│   │   ├── Oscilloscope.h/cpp        # 示波器缓冲区
│   │   └── LUFSMeter.h/cpp           # EBU R128响度测量
│   ├── recorder/
│   │   └── AudioRecorder.h/cpp       # WAV录制
│   └── gui/
│       ├── ChannelStrip.h/cpp        # 主面板布局
│       ├── MeterPanel.h/cpp          # 电平表+相位表+示波器
│       ├── RecordingPanel.h/cpp      # 录制控制
│       ├── theme/
│       │   ├── UTTLookAndFeel.h      # 主题基类
│       │   ├── ChannelStripLNF.h/cpp # 默认: 通道条硬件
│       │   ├── DarkModernLNF.h/cpp   # 暗色现代
│       │   ├── RetroLightLNF.h/cpp   # 亮色复古
│       │   └── ThemeLoader.h/cpp     # JSON主题文件加载
│       └── controls/
│           ├── VUKnob.h/cpp          # VU风格旋钮
│           ├── VUMeter.h/cpp         # VU表
│           ├── LedButton.h/cpp       # LED指示灯按钮
│           └── SegmentDisplay.h/cpp  # 段式数字显示
└── themes/
    ├── channel_strip.json            # 默认主题配置
    ├── dark_modern.json
    └── retro_light.json
```

---

## 参数清单 (共 ~35 个)

### DSP 处理参数
| # | ID | 类型 | 范围 | 默认 |
|---|---|---|---|---|
| 1 | dcOffsetBypass | bool | — | true |
| 2 | phaseInvertL | bool | — | false |
| 3 | phaseInvertR | bool | — | false |
| 4 | phaseOffset | float | 0~360° | 0 |
| 5 | gain | float | -96~24 dB | 0 |
| 6 | msMode | int | 0=Bypass/1=Encode/2=Decode | 0 |
| 7 | monoMode | int | 0=Off/1=L+R/2=Mid/3=Side | 0 |
| 8 | stereoSeparation | float | 0~100% | 100 |
| 9 | monoToStereo | int | 0=Off/1=L→Both/2=R→Both | 0 |
| 10 | channelSwap | bool | — | false |
| 11 | pan | float | -100~+100 | 0 |

### Output Stage 参数
| # | ID | 类型 | 范围 | 默认 |
|---|---|---|---|---|
| 12 | outputLGain | float | -96~12 dB | 0 |
| 13 | outputRGain | float | -96~12 dB | 0 |
| 14 | outputLMute | bool | — | false |
| 15 | outputRMute | bool | — | false |
| 16 | outputLSolo | bool | — | false |
| 17 | outputRSolo | bool | — | false |

### 测量输出参数 (只读)
| # | ID | 类型 | 范围 |
|---|---|---|---|
| 18 | meterLPeak | float | dBFS |
| 19 | meterRPeak | float | dBFS |
| 20 | meterLRMS | float | dBFS |
| 21 | meterRRMS | float | dBFS |
| 22 | phaseCorrelation | float | -1~+1 |
| 23 | lufsMomentary | float | LUFS |
| 24 | lufsShortTerm | float | LUFS |
| 25 | lufsIntegrated | float | LUFS |

### 录制参数
| # | ID | 类型 | 范围 | 默认 |
|---|---|---|---|---|
| 26 | recordEnable | bool | — | false |
| 27 | recordPath | String | 文件路径 | "" |

### GUI 参数
| # | ID | 类型 | 范围 | 默认 |
|---|---|---|---|---|
| 28 | themeIndex | int | 0/1/2 | 0 |
| 29 | customThemePath | String | JSON路径 | "" |

---

## Solo 逻辑规则

Solo 优先级高于 Mute。具体行为：

- 无 Solo 激活时：L Mute 静音 L，R Mute 静音 R
- 仅 L Solo 激活时：输出 L，R 静音（无论 R Mute 状态）
- 仅 R Solo 激活时：输出 R，L 静音（无论 L Mute 状态）
- 两个 Solo 同时激活时：行为等同于无 Solo（正常输出）

---

## 主题系统设计

### 内置主题

1. **ChannelStripLNF (默认)** — 模拟 SSL 9000/API 2500 风格
   - 深灰色金属拉丝背景
   - VU 表带背光指针
   - 大号金属旋钮 (仿 API 风格帽盖)
   - 红色/绿色 LED 指示灯
   - 段式 LED 电平显示
   - 字体: 等宽工业风格 (如 Eurostile 替代)

2. **DarkModernLNF** — 扁平暗色现代风格
   - 深灰/炭黑背景
   - 细线条电平条
   - 简洁圆形旋钮
   - 蓝/青色强调色

3. **RetroLightLNF** — 复古亮色风格
   - 奶油/米白背景
   - 仿 Neve 风格蓝色/红色旋钮帽
   - 传统 VU 表
   - 暖色调强调

### 外部主题 JSON Schema

```json
{
  "name": "My Theme",
  "colors": {
    "background": "#2A2A2A",
    "panelBackground": "#1E1E1E",
    "textPrimary": "#E0E0E0",
    "accent": "#FF6600",
    "vuNeedle": "#FF4400",
    "vuBacklight": "#FFCC00",
    "meterLow": "#00FF00",
    "meterMid": "#FFFF00",
    "meterHigh": "#FF0000",
    "buttonOn": "#FF0000",
    "buttonOff": "#333333",
    "knobCap": "#C0C0C0",
    "knobBase": "#555555"
  },
  "fonts": {
    "main": "Eurostile",
    "mono": "Consolas",
    "display": "DS-Digital"
  },
  "backgroundImage": "path/to/bg.png",
  "knobStyle": "api"
}
```

ThemeLoader 解析 JSON，创建对应的 LookAndFeel 子类实例，应用全部颜色/字体/样式。

---

## 开发阶段

### Phase 1 — 项目框架搭建
- Clone JUCE 作为 git submodule
- 创建 CMakeLists.txt (juce_add_plugin)
- 创建最小 PluginProcessor/PluginEditor (直通音频)
- 验证编译 + DAW 加载

### Phase 2 — 全部 DSP 模块
- 按信号链路顺序逐一实现每个模块
- 每模块独立单元测试 (juce::UnitTest)
- Output Stage 含 Solo/Mute 逻辑

### Phase 3 — 测量系统
- Peak/RMS 电平表，Phase Correlation Meter
- Oscilloscope 环形缓冲
- LUFS (EBU R128) — Momentary/Short-term/Integrated

### Phase 4 — 录制
- WAV 文件写入 (juce::WavAudioFormat)
- 录制开关 + 文件路径管理

### Phase 5 — GUI 与主题
- 主面板布局 (通道条三栏: 处理 | 测量 | 录制)
- 自定义控件 (VU旋钮/VU表/LED按钮/段式显示)
- 3 套内置 LookAndFeel 实现
- JSON 主题加载器
- 主题切换参数

### Phase 6 — 集成测试与打磨
- 全链路信号测试
- DAW 自动化参数测试
- 主题切换无闪烁
- 性能 profiling (CPU 占用)

---

## 关键假设

- **Phase Offset** 默认实现一阶全通滤波器 (固定频点相位偏移)。全频段精确度数偏移需 Hilbert/FFT 方案，作为 v2 迭代。
- **LUFS Integrated** 从播放开始累计到传输停止，需持有全程历史数据；进程内内存足够（~采样率×声道×4字节×时长）。
- **外部主题 JSON** 加载失败时回退到默认 Channel Strip 主题并弹出警告。
- **JUCE submodule** 使用 `juce` 官方 GitHub 仓库 `main` 分支，编译目标为 VST3。如需其他格式由 JUCE CMake 配置控制。
- **录制路径** 初始为空，用户通过文件选择器设置；路径无效时录制自动禁用。
