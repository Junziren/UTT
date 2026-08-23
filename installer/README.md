# UTT Windows Installer

`build-installer.ps1` packages the canonical Release VST3 bundle, Standalone
executable, and a single-file `UTT_Setup.exe` installer.

The setup executable embeds these signed Microsoft prerequisites:

- WebView2 Evergreen Standalone Runtime x64
- Visual C++ 2015-2022 Redistributable x64

It installs only `UTT.vst3` into the standard system VST3 location:

```text
C:\Program Files\Common Files\VST3\UTT.vst3
```

## Build

From the repository root, first build the canonical Release artefacts:

```powershell
cmake --build build --target UTT_All UTTTests -j 4
```

Then create the installer and release folder:

```powershell
.\installer\build-installer.ps1 -DownloadPrerequisites
```

The output is written to the CMake product version directory:

```text
dist\UTT-<version>-windows-x64\
```

`SHA256SUMS.txt` covers every shipped file, including `LICENSE.txt`. The
script also creates `dist\UTT-<version>-windows-x64.zip` and an adjacent
`.sha256` file for GitHub Releases. The temporary payload directory and release
output are deliberately not tracked by Git.

## Validation Boundary

The build script verifies Microsoft Authenticode signatures before embedding
prerequisites, checks the extracted VST3 module path, and hashes the final ZIP.
A clean offline VM, code signing, and DAW smoke tests remain separate release
gates.
