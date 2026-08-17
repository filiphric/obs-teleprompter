# OBS Teleprompter

Read naturally on camera without looking away from OBS. Keep your scripts as simple Markdown files, make last-minute edits in place, and present them fullscreen at a pace that feels comfortable.

<p align="center">
  <a href="https://github.com/filiphric/obs-teleprompter/releases/latest/download/OBS-Teleprompter-macOS-Universal.pkg"><img alt="Download for macOS" src="https://img.shields.io/badge/Download-macOS-111111?style=for-the-badge&logo=apple"></a>
  <a href="https://github.com/filiphric/obs-teleprompter/releases/latest/download/OBS-Teleprompter-Windows-x64.zip"><img alt="Download for Windows" src="https://img.shields.io/badge/Download-Windows-0078D4?style=for-the-badge&logo=windows11"></a>
</p>

![OBS Teleprompter with a sample script](https://github.com/filiphric/obs-teleprompter/releases/latest/download/obs-teleprompter.png)

Open **Tools → Teleprompter** in OBS, choose the folder where you keep your scripts, and start presenting. The macOS installer is unsigned; if macOS blocks it, allow it in **System Settings → Privacy & Security**. On Windows, extract the ZIP into `%APPDATA%\obs-studio\plugins`.

## Contributing

You’ll need CMake 3.28+, a C++17 compiler, and the platform tools listed by the [OBS plugin template](https://github.com/obsproject/obs-plugintemplate). Dependencies are downloaded automatically.

```sh
cmake --preset macos
cmake --build --preset macos
```

On Windows, use the `windows-x64` preset. Versions live in `VERSION.txt` and `buildspec.json`; update both with `CHANGELOG.md` before tagging a release.
