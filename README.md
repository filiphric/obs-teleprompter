# OBS Teleprompter

A native OBS Studio plugin that opens a teleprompter in its own window. Scripts are read from a selected folder and rendered from Markdown.

## First version

- Open it from **Tools → Teleprompter** in OBS.
- Choose a folder containing `.md` or `.markdown` files.
- Select a script from the list.
- Edit the selected script directly in the window and save with **Cmd+S**.
- Press **Cmd+Enter** or click **Start** to enter fullscreen and begin scrolling.
- In fullscreen, press **Space** to pause or resume without losing your position.
- Press **Escape** to stop and return to the editor.
- Press **Left / Right** to decrease or increase scrolling speed by 10 px/s.
- Press **Up / Down** to jump by one third of the visible page.
- Press **Option+Left / Right** to adjust horizontal margins by 20 px.
- Press **Option+Up / Down** to adjust fullscreen text size.
- Paused fullscreen controls overlay the script without shifting its position.
- Fullscreen lead-in and ending space place the first and last lines near screen center.
- Sort scripts by name or last-modified date.
- Markdown headings, lists, emphasis, links, and paragraph spacing are preserved.
- The folder, speed, margin, and sorting settings persist between sessions.

## Build

This project uses the official OBS plugin build infrastructure and requires CMake 3.28+, a C++17 compiler, and the platform tools listed by the [OBS plugin template](https://github.com/obsproject/obs-plugintemplate). Pinned OBS and Qt dependencies are downloaded automatically.

```sh
cmake --preset macos
cmake --build --preset macos
```

On Windows, use the `windows-x64` preset instead. Restart OBS after installing the plugin.

## Install a release

Download the file for your computer from [GitHub Releases](https://github.com/filiphric/obs-teleprompter/releases):

- **macOS (Apple Silicon or Intel):** `OBS-Teleprompter-<version>-macOS-Universal.pkg`
- **Windows 64-bit:** `OBS-Teleprompter-<version>-Windows-x64.zip`
- Files containing `Source` are for developers and are not ready-to-install plugins.

On Windows, extract the ZIP into `%APPDATA%\obs-studio\plugins`, preserving the included folder structure. On macOS, open the PKG installer. The macOS package is currently unsigned, so macOS may require confirmation in **System Settings → Privacy & Security**.

## Versions and releases

This project follows [Semantic Versioning](https://semver.org/). The current version lives in [`VERSION.txt`](VERSION.txt), is mirrored in `buildspec.json` for the OBS packaging tools, and is validated by CMake and the release workflow.

To publish a release:

1. Update `VERSION.txt`, the `version` in `buildspec.json`, and `CHANGELOG.md` in a pull request or commit.
2. Tag that commit with the matching version, for example `v0.2.0`.
3. Push the tag: `git push origin v0.2.0`.

The release workflow validates the tag, builds macOS Universal and Windows x64 packages, creates explicitly named source archives, generates SHA-256 checksums, and publishes a GitHub Release with generated notes. Tags and versions must use `MAJOR.MINOR.PATCH`; increment the major version for incompatible changes, minor for backward-compatible features, and patch for backward-compatible fixes.

## Roadmap

Scene overlays and other broadcast integrations are intentionally left for a later version.
