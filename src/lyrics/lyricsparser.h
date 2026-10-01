// lyricsparser.h - the boundary between external lyric formats and the model.
//
// A parser reads the bytes of one specific format (LRC, TTML, an embedded tag,
// ...) and produces a `lyrics::Lyrics`. This header fixes the shape so parsers
// are interchangeable and the model stays independent of any one format.
//
//   external format -> Parser -> lyrics::Lyrics
//
// A parser allocates ids through the `Lyrics::add*` helpers, which keeps
// cross-references (agents, translation source lines, transliteration sources)
// consistent without the parser ever handling raw pointers.
//
// ---------------------------------------------------------------------------
// Worked examples (documentation only - not implemented here)
// ---------------------------------------------------------------------------
//
// Plain LRC. A line only carries a start; the end is implied by the next one,
// so it is left empty rather than invented.
//
//   [00:12.50]Hello world
//
//   lyrics::Lyrics lyrics;
//   lyrics::Line &line = lyrics.addLine();
//   line.mainVocal.text = QStringLiteral("Hello world");
//   line.mainVocal.timing = lyrics::Timing{std::chrono::milliseconds(12500)};
//
// Enhanced LRC with word timing. Only the anchors the file gives are filled in.
//
//   [00:12.50]<00:12.50>Hello <00:13.10>world
//
//   lyrics::Line &line = lyrics.addLine();
//   line.mainVocal.timing = lyrics::Timing{std::chrono::milliseconds(12500)};
//   line.mainVocal.text = QStringLiteral("Hello world");
//   line.mainVocal.words.push_back(lyrics::Word{
//       QStringLiteral("Hello"), lyrics::Timing{std::chrono::milliseconds(12500)}, {}});
//   line.mainVocal.words.push_back(lyrics::Word{
//       QStringLiteral("world"), lyrics::Timing{std::chrono::milliseconds(13100)}, {}});
//
// Multiple singers. Agents are registered once and referenced by id, so several
// lines can share the same voice.
//
//   const lyrics::Id alice = lyrics.addAgent(lyrics::AgentType::Person, "Alice");
//   const lyrics::Id group = lyrics.addAgent(lyrics::AgentType::Group, "Choir");
//   lyrics.addLine().mainVocal.agentId = alice;
//   lyrics.addLine().mainVocal.agentId = group;
//
// Background vocals belong to their line and time independently.
//
//   lyrics::Line &line = lyrics.addLine();
//   line.mainVocal.text = QStringLiteral("I don't wanna know");
//   lyrics::Vocal &echo = line.backgrounds.emplace_back();
//   echo.text = QStringLiteral("(don't wanna know)");
//
// Translations reuse the source line's timing, so a translation stores only the
// relationship and its own text.
//
//   lyrics::Line &line = lyrics.addLine();
//   line.mainVocal.text = QStringLiteral("ありがとう");
//   line.translations.push_back(lyrics::Translation{
//       QStringLiteral("fa"), QStringLiteral("ممنون"), line.id()});
//
// Transliteration is separate from translation and points at the original.
//
//   line.transliterations.push_back(lyrics::Transliteration{
//       QStringLiteral("arigatou"), line.id()});

#pragma once

#include "lyrics.h"

#include <optional>

#include <QByteArray>
#include <QString>

namespace lyrics {

/// The outcome of a parse. On failure `lyrics` is empty and `error` explains
/// why; on success `error` is empty.
struct ParseResult {
    std::optional<Lyrics> lyrics;
    QString error;

    bool succeeded() const { return lyrics.has_value(); }
};

/// Converts one external lyric format into the model.
class Parser {
  public:
    virtual ~Parser() = default;

    /// Short identifier of the format, e.g. "lrc" or "ttml".
    virtual QString formatName() const = 0;

    /// Parses `data` and returns either the model or an error. Implementations
    /// must never throw for malformed input.
    virtual ParseResult parse(const QByteArray &data) const = 0;
};

} // namespace lyrics
