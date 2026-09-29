# Karaoke

A Qt 6 / QML music player: it scans your music folder, plays the files with
GStreamer, reads tags and lyrics with TagLib, and shows the lyrics as plain
text (synced, GPU-highlighted lyrics are the next phase).

The app opens on a **songs list**. Tapping a track starts a library queue and
opens the **now-playing** screen: the cover and transport in the middle, the
**lyrics** pane on the right and the **queue** on the left. The two side panes
are mutually exclusive.

## Build & run

```sh
cmake -S . -B build -G Ninja
cmake --build build
./build/karaoke
```

Requires Qt 6.5+ (Quick, Qml, ShaderTools), GStreamer 1.0 (`gstreamer-1.0`) and
TagLib 2.x, all found through `pkg-config`. Shaders are compiled to `.qsb` at
build time, so nothing generated is committed.

## Music folder

The library is scanned from `QStandardPaths::MusicLocation` (your XDG music
directory, e.g. `~/mus`), recursively. If that does not exist it falls back to
`~/Music`. For each track TagLib reads title/artist/album/duration and embedded
lyrics; a sidecar `.lrc`/`.ttml` file with the same base name wins over the
embedded tag. Lyrics are shown as extracted plain text for now - no timing yet.

## Controls

- **Songs list** — click a row to play it and open the now-playing screen
- **Now-playing** — `Prev` / `Play` / `Next`, a click-to-scrub seek bar, and the
  `Queue` / `Repeat` / `Shuffle` toggles (repeat cycles off → all → one)
- **Queue** — opens on the left, lists the play queue; click a row to jump to it
- **Space** — pause / resume

Two environment variables help when testing without a sound device or window:

- `KARAOKE_AUDIO_SINK=fakesink` — pick the GStreamer audio sink
- `QT_QPA_PLATFORM=offscreen` — run without a display

## How it works

Playback is one GStreamer `playbin`, driven from the Qt event loop: a timer
polls the bus for errors/end-of-stream and queries the position, so no GLib main
loop is needed. The player also owns the queue, repeat and shuffle.

`MusicLibrary` scans off the event loop a chunk at a time, so the list fills in
without blocking the UI.

```
source file ──TagLib──▶ Song (title/artist/duration/lyrics)
            ──playbin─▶ audio out
```

The dormant karaoke renderer is still in the repo for the synced-lyrics phase:

```
Flow of Text words     -> ShaderEffectSource   the glyphs, as one texture
Flow of word gradients -> ShaderEffectSource   each fragment's reading position
ShaderEffect           -> recolours along the reading order
```

A line may wrap onto several rows; each word paints an opaque gradient giving
that pixel's position along the reading order, so the shader compares it with
`progress` instead of raw `x`.

Navigation is a single `StackView`: `SongsPage` is the root and pushes
`NowPlayingPage`.

| File | Role |
| --- | --- |
| `song.h/.cpp` | `Song` (track metadata + lyrics) and the `MusicLibrary` scanner |
| `player.h/.cpp` | GStreamer playback, queue, repeat and shuffle |
| `qml/Main.qml` | window shell, `StackView` navigation, starts the scan |
| `qml/SongsPage.qml` | the songs list (main page) |
| `qml/NowPlayingPage.qml` | cover, transport and the queue/lyrics panes |
| `qml/QueueView.qml` | the play queue list |
| `qml/LyricsText.qml` | plain-text lyrics view |
| `qml/Artwork.qml` | a track's gradient cover |
| `qml/KaraokeLine.qml`, `qml/LyricsView.qml` | dormant synced-lyrics renderer |
| `shaders/` | the GPU sweep used by the dormant renderer |
