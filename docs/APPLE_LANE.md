# Apple 车道（AUv3 生态对齐）—— 预备文档

> **状态：UNVERIFIED。** 本机为 Windows。本文档描述的一切 Apple 侧内容都只是
> 代码与构建预备，**没有任何 macOS/iOS 构建或真机证据**。
> 「Windows 编译通过 + pluginval 通过 ≠ AUv3 已验证。」
> Apple 证据只可以在 Mac/Xcode 环境产生，然后经 commit/PR 回流到本仓库。

## 1. 已完成的预备（Windows 侧可验证部分）

- CMake 格式矩阵（`CMakeLists.txt`）：
  - Windows/Linux：`VST3 + Standalone`
  - macOS：`VST3 AU AUv3`（AUv3 产出 `.appex`）
  - iOS：`-DCMAKE_SYSTEM_NAME=iOS` 时 `AUv3 + Standalone`
- 标识符冻结：见 `docs/IDENTIFIERS.md`（bundle id 规划、aufx、参数 ID ABI）。
- 沙盒安全整改：
  - CrashLog/trace 全部改写到 `~/Library/Application Support/UnpureBloom/UTT/`
    （不再写桌面；AUv3 扩展容器内为应用容器目录）。
  - 录音默认路径走 `userMusicDirectory`；iOS 上需要 security-scoped bookmark
    与「文件 App 共享」配置（见 §4 待办）。
- 扩展生命周期：
  - 录音 writer 线程生命周期挂接 `prepareToPlay/releaseResources`，无后台常驻假设。
  - 示波器/LUFS 等分析状态随 `reset()` 清零，不假设进程存活。
- WebView 双后端声明：Windows 显式 `Backend::webview2`；Apple 上
  `WebBrowserComponent` 默认后端即 WKWebView，资源经 `ResourceProvider`
  白名单提供，不依赖本地 HTTP。

## 2. macOS 构建（在 Mac 上执行）

```bash
cmake -B build-mac -G Xcode -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac --config Release --target UTT
```

产出：
- `build-mac/UTT_artefacts/Release/VST3/UTT.vst3`
- `.../AU/UTT.component`（AUv2）
- `.../AUv3/UTT.appex`（需嵌入宿主应用才可注册）

注册验证：
```bash
auval -v aufx UT02 UT01     # AUv2 验证必须先过
```

## 3. iOS 构建（在 Mac 上执行）

```bash
cmake -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DCMAKE_BUILD_TYPE=Release
```

AUv3 on iOS 必须有容器 App：需要新建一个宿主 App target，把 `UTT.appex`
嵌入，并在 Xcode 中配置：
- App Group（扩展与容器共享录制文件暂存区）
- entitlements：`com.apple.security.application-groups`
- Info.plist：`NSExtension → NSExtensionPointIdentifier com.apple.audio-unit`
  （JUCE 生成，需核对）

## 4. 验收清单（Apple 车道，全部待做）

构建/签名：
- [ ] macOS `auval -v aufx UT02 UT01` 通过
- [ ] AUv3 appex 嵌入测试容器后被系统注册
- [ ] 签名 + notarization 通过（分发形态）
- [ ] iOS 真机 build & install

宿主矩阵（至少）：
- [ ] GarageBand macOS：扫描、实例化、编辑器打开（WKWebView UI）、参数自动化
- [ ] GarageBand iOS：同上 + 触控目标 ≥44px 检查
- [ ] 至少一个其他 AUv3 宿主（AUM / Audiobus / Logic Pro iPad 任一）

行为专项：
- [ ] 录音写入容器可见位置并通过文件 App 可取（iOS）
- [ ] 内存警告下编辑器释放后重开不泄漏（WKWebView 进程数稳定）
- [ ] 前后台切换：音频不中断、编辑器状态正确恢复
- [ ] 无 UI 渲染（宿主关闭编辑器）时音频与录音继续正常
- [ ] WKWebView 资源全部来自内嵌 BinaryData（断网测试）
- [ ] 宿主自动化写参数 → UI 只更新显示，不回传手势

## 5. 已知风险（写代码时已规避，但需 Apple 实证）

| 风险 | 规避方式 | 需验证点 |
|---|---|---|
| AUv3 扩展进程内存上限 | UI 资源全量打包、无位图主题 | 真机内存警告测试 |
| WKWebView 与 WebView2 行为差异 | 桥合同小且版本化；无平台私有 API | 双端手势/事件回归 |
| 沙盒写路径 | CrashLog/录音路径已整改 | iOS 文件 App 取录音 |
| 扩展被宿主随时杀死 | 录音 FIFO 音频线程只入队；writer 线程生命周期挂 prepare/release | 杀进程后文件完整性 |
| App Group 缺失导致共享失败 | 规划在 §3 | entitlements 实配 |

## 6. 证据回流格式

在 Mac 上完成任何一项后，以 PR 形式追加到本文件 §4 勾选并附：
机器/OS 版本、Xcode 版本、宿主与版本、日期、截图或日志路径。
未附证据的勾选无效。
