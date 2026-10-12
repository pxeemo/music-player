// ttmlparser.h - a basic TTML parser.
//
// TTML is XML, and the shape a music player meets is the Apple Music one:
//
//   <head><metadata>
//     <ttm:agent xml:id="v1" type="person"/>              who is singing
//   </metadata></head>
//   <body>
//     <p begin="00:12.340" end="00:15.000" ttm:agent="v1">Plain line</p>
//     <p begin="..." end="..."><span begin="..." end="...">word</span> ...</p>
//     <p begin="..." end="..." ttm:role="x-bg">Backing vocal</p>
//   </body>
//
// A paragraph is one lyric line. A `<span>` that times itself is a word, and
// spans glued together without whitespace between them are syllables of one
// word - the same reading the LRC parser gives its `<mm:ss.xx>` tags, so
// scripts that join stay in one piece. A paragraph with no spans stays a plain
// line rather than pretending to be karaoke, and a paragraph with no `end`
// keeps an empty one for the document to derive from the next line.
//
// Not read yet: section markers (`itunes:songPart`), the head metadata beyond
// agents, `<audio lyricOffset>` and `leadingSilence` (what they shift is not a
// guess worth making), and splitting a paragraph at `<br/>` - a break there
// only separates the words it sits between.

#pragma once

#include "lyricsparser.h"

#include <QString>

namespace lyrics {

class TtmlParser : public Parser {
  public:
    QString formatName() const override;
    ParseResult parse(const QByteArray &data) const override;
};

} // namespace lyrics
