# Karaoke

A Qt 6 / QML music player: it scans your music folder, plays the files with
GStreamer, reads tags and lyrics with TagLib, and parses timing so the lyrics
follow the song - line by line, and word by word when the source is enhanced.

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
embedded tag.

Lyric sources are parsed by format. A `.lrc` (or embedded text that looks like
one) goes through `LrcParser`: metadata tags, `[mm:ss.xx]` line timestamps and
enhanced `<mm:ss.xx>` word timestamps, with each line's end taken from the next.
Embedded lyrics have no extension, so the format is guessed from the text. TTML
is not parsed yet - those fall back to plain text.

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
source file ──TagLib──▶ tags + lyric text
            ──Parser──▶ lyrics::Lyrics model
            ──playbin─▶ audio out
```

`LyricsView` draws that model. Each paragraph is rendered on its own terms: an
untimed line is plain text, a line with only line-level timing fades between
unsung and sung colours, and word- or syllable-level timing switches to the
synced GPU sweep:

```
Flow of Text words     -> ShaderEffectSource   the glyphs, as one texture
Flow of word gradients -> ShaderEffectSource   each fragment's reading position
ShaderEffect           -> recolours along the reading order
```

A line may wrap onto several rows; each word paints an opaque gradient giving
that pixel's position along the reading order, so the shader compares it with
`progress` instead of raw `x`. Background vocals and translations are drawn as
their own smaller rows, and clicking a timed row seeks to it.

Navigation is a single `StackView`: `SongsPage` is the root and pushes
`NowPlayingPage`.

| File | Role |
| --- | --- |
| `lyricsmodel.h/.cpp` | format-independent `Lyrics` model: elements, timing, agents |
| `lyricsparser.h` | the `Parser` interface every lyric format implements |
| `lrcparser.h/.cpp` | basic LRC parser (metadata, line + enhanced word timing) |
| `lyricsdocument.h/.cpp` | flattens the model into rows for QML |
| `song.h/.cpp` | `Song` (track metadata + parsed lyrics) and the `MusicLibrary` scanner |
| `player.h/.cpp` | GStreamer playback, queue, repeat and shuffle |
| `qml/Main.qml` | window shell, `StackView` navigation, starts the scan |
| `qml/SongsPage.qml` | the songs list (main page) |
| `qml/NowPlayingPage.qml` | cover, transport and the queue/lyrics panes |
| `qml/QueueView.qml` | the play queue list |
| `qml/LyricsView.qml` | the line-by-line / synced lyric sheet |
| `qml/LyricsText.qml` | plain scrollable text (used for the raw parser dump) |
| `qml/KaraokeLine.qml` | one synced line: GPU highlight sweep |
| `qml/Artwork.qml` | a track's gradient cover |
| `shaders/` | the GPU sweep used by `KaraokeLine` |
