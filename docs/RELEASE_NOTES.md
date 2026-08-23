## UTT release assets

- Windows x64: VST3 bundle, Standalone executable, and `UTT_Setup.exe` are inside the ZIP.
- macOS universal: the unsigned `.pkg` installs VST3, AU, and `/Applications/UTT.app`; the app contains the AUv3 extension.
- Each archive/package has an adjacent SHA-256 file.

## Validation boundary

Both CI lanes build Release artefacts and run `UTTTests`. The macOS package is not Developer ID signed and not notarized because this repository has no Apple signing credentials; its contained VST3, AU, and AUv3 container app are ad-hoc signed. Gatekeeper approval and host validation (`auval`, DAW/AUv3 host smoke tests) remain required before a signed production distribution.
