// lyricsserializer.h - the model as JSON, so a parsed document can be stored.
//
// The library cache remembers lyrics it has already parsed instead of reading
// them from the file again; that needs the model in a form SQLite can hold, and
// JSON is the simplest thing that round-trips exactly: ids, optional timing at
// every level and every element kind survive a write and a read unchanged.
//
// Reading is strict rather than forgiving: anything that does not read back the
// way it was written - a different version, a missing field, an id that no
// longer lines up - yields nothing at all, and the caller reads the source file
// again.
#pragma once

#include "lyrics.h"

#include <QByteArray>
#include <optional>

namespace lyrics {

/// The layout `toJson()` writes; a reader refuses any other version. The
/// library cache folds this into its schema version, so raising it (as must
/// happen whenever the layout changes) makes every stored row be read from its
/// source file again on the next scan.
inline constexpr int JsonVersion = 1;

/// The document as compact JSON.
QByteArray toJson(const Lyrics &document);

/// The document `toJson()` produced, or nothing when `json` is not one.
std::optional<Lyrics> fromJson(const QByteArray &json);

} // namespace lyrics
