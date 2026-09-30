// lrcparser.h - a basic LRC parser.
//
// Handles the LRC pieces a music player actually meets:
//   [ti:...] [ar:...] [al:...] [offset:...]   metadata
//   [mm:ss.xx]text                            one or more timestamps per line
//   [mm:ss.xx]<mm:ss.xx>word ...              enhanced (word-level) timing
//
// Line ends are not stored by LRC; they are derived from the next timed line,
// which is the usual reading of the format. Untimed text lines are kept as
// untimed lines. Nothing here knows about TTML or any other format.

#pragma once

#include "lyricsparser.h"

#include <QString>

namespace lyrics {

class LrcParser : public Parser {
  public:
    QString formatName() const override;
    ParseResult parse(const QByteArray &data) const override;

    /// Cheap sniff used when lyrics have no file extension (e.g. an embedded
    /// tag): true when the text contains at least one `[mm:ss.xx]` marker.
    static bool looksLikeLrc(const QString &text);
};

} // namespace lyrics
