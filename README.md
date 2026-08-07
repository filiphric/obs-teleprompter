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

This project requires the OBS Studio development libraries, the OBS frontend API, Qt 6, CMake 3.20+, and a C++17 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo
cmake --install build --config RelWithDebInfo
```

Depending on how OBS was installed, pass `CMAKE_PREFIX_PATH` so CMake can find OBS and Qt. Restart OBS after installing the plugin.

## Roadmap

Scene overlays and other broadcast integrations are intentionally left for a later version.
