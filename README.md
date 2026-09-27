# Karaoke

A Qt 6 / QML demo of Apple-Music-style karaoke lyrics: each line is rendered
once into a texture and recoloured left to right entirely on the GPU.

The app opens on a **songs list**. Tapping a song opens its **now-playing**
screen, where the cover sits on the left and a **collapsible lyrics panel** sits
next to it on the right.

## Build & run

```sh
cmake -S . -B build -G Ninja
cmake --build build
./build/karaoke
```

Requires Qt 6.5+ (Quick, Qml, ShaderTools). Shaders are compiled to `.qsb` at
build time, so nothing generated is committed.

## Controls

- **Songs list** — click a row to open the now-playing screen
- **Now-playing** — click the handle on the lyrics panel's right edge to
  collapse/expand it; click the seek bar to scrub
- **Space** — pause / resume (while a song is open)
- **Mouse wheel or drag** — scroll the lyrics by hand
- The lyrics recenter themselves whenever a line starts being sung

## How it works

```
Text (one static Text per word, laid out in a Row)
  -> ShaderEffectSource   the whole line captured as one texture
  -> ShaderEffect         GPU recolours it from left to right
```

Nothing about the text is rebuilt or recoloured on the CPU. The only value that
changes per frame is `progress`, a normalised 0..1 position interpolated from
the word timings against the laid-out word positions.

Navigation is a single `StackView`: `SongsPage` is the root and pushes
`NowPlayingPage` with the tapped `Song`.

| File | Role |
| --- | --- |
| `lyrics.h/.cpp` | the timing model: `Word`, `LyricLine` |
| `song.h/.cpp` | `Song` (metadata + own lyrics) and the `SongsModel` catalogue |
| `playbackclock.h/.cpp` | one 16 ms clock driving every line |
| `qml/Main.qml` | window shell and `StackView` navigation |
| `qml/SongsPage.qml` | the songs list (main page) |
| `qml/NowPlayingPage.qml` | cover, transport and the collapsible lyrics panel |
| `qml/LyricsPanel.qml` | collapse/expand container + handle |
| `qml/LyricsView.qml` | the scrolling lyric sheet for one song |
| `qml/Artwork.qml` | a song's gradient cover |
| `qml/KaraokeLine.qml` | one line: layout, activation fade, size swell, `progress` |
| `shaders/` | the sweep itself (`.qsb` built by CMake) |

Appearance is tunable from QML (`KaraokeLine` exposes the colours, `edge`,
`glow`, `sizeBoost`, `transitionDuration`); `SongsModel::loadDemoSongs()` is the
only place the hard-coded songs live and is the seam a real library/LRC parser
would fill.
