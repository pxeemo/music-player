# Karaoke

A Qt 6 / QML demo of Apple-Music-style karaoke lyrics: each line is rendered
once into a texture and recoloured left to right entirely on the GPU.

## Build & run

```sh
cmake -S . -B build -G Ninja
ninja -C build
./build/karaoke
```

Requires Qt 6.5+ (Quick, Qml, ShaderTools). Shaders are compiled to `.qsb` at
build time, so nothing generated is committed.

## Controls

- **Space** — pause / resume
- **Mouse wheel or drag** — scroll the lyrics by hand
- The view recenters itself whenever a line starts being sung

## How it works

```
Text (one static Text per word, laid out in a Row)
  -> ShaderEffectSource   the whole line captured as one texture
  -> ShaderEffect         GPU recolours it from left to right
```

Nothing about the text is rebuilt or recoloured on the CPU. The only value that
changes per frame is `progress`, a normalised 0..1 position interpolated from
the word timings against the laid-out word positions.

| File | Role |
| --- | --- |
| `lyrics.h/.cpp` | the timing model: `Word`, `LyricLine`, `LyricsModel` |
| `playbackclock.h/.cpp` | one 16 ms clock driving every line |
| `qml/Main.qml` | window, scrolling lyric view, transport controls |
| `qml/KaraokeLine.qml` | one line: layout, activation fade, size swell, `progress` |
| `shaders/` | the sweep itself (`.qsb` built by CMake) |

Appearance is tunable from QML (`KaraokeLine` exposes the colours, `edge`,
`glow`, `sizeBoost`, `transitionDuration`); `loadDemoLyrics()` is the only place
the hard-coded timings live and is the seam a real LRC/TTML parser would fill.
