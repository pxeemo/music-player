// trackloader.h - reads one audio file into a Song.
//
// This is where tags, embedded/sidecar lyrics and cover art come together:
// TagLib for the tags and pictures, the lyrics parsers for the text, and the
// artwork cache for the extracted image. The library scanner calls loadTrack()
// once per file.

#pragma once

#include <QString>
#include <QStringList>

class Song;

namespace library {

class LibraryCache;

/// Name filters for the audio formats the scanner picks up.
QStringList audioNameFilters();

/// Fills `song` from the file at `path`: tags, duration, artwork and lyrics.
/// Never throws; missing pieces are simply left empty. When `cache` still holds
/// the file, everything comes from the row instead of from the file itself -
/// except with `refresh`, which reads the file again and replaces the row, for
/// a cache entry that turned out to be unreadable.
void loadTrack(const QString &path, Song *song, LibraryCache *cache = nullptr,
               bool refresh = false);

} // namespace library
