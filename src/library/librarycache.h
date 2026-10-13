// librarycache.h - the scanned library, remembered in SQLite.
//
// MusicLibrary rescans its folder every time the app starts, which means
// opening every file with TagLib again. This remembers everything
// library::loadTrack() learned about a file - tags, cover, stamps and the
// parsed lyrics document - so a rescan only has to walk the directory and
// compare stamps.
//
// A row is used only while the audio file, its sidecar lyric file (when there
// is one) and its extracted cover are still exactly what the row was written
// from; anything else falls back to reading the file, and the row is rewritten.
// Rows for files that disappeared are dropped when the scan ends, which is why
// beginScan()/endScan() bracket it.
//
// The tables carry `PRAGMA user_version`: a database written by a different
// schema is dropped and rebuilt, never migrated. Every call is a no-op when the
// database could not be opened, so callers never have to care.
#pragma once

#include <QByteArray>
#include <QSqlDatabase>
#include <QString>

namespace library {

/// One track as the cache remembers it: exactly what loadTrack() read.
struct CachedTrack {
    qint64 mtimeMs = -1;
    qint64 sizeBytes = -1;
    QString title;
    QString artist;
    QString album;
    qreal duration = 0.0;
    QString artwork; ///< cover URL, empty when the track has no cover
    qint64 addedMs = -1;
    QString sidecar; ///< lyric file the document came from, empty when none
    qint64 sidecarMtimeMs = -1;
    QByteArray lyrics; ///< lyrics::toJson() of the parsed document
};

/// True while `track` still describes a file with these stamps.
bool isFresh(const CachedTrack &track, qint64 mtimeMs, qint64 sizeBytes, qint64 sidecarMtimeMs);

/// The SQLite store for CachedTrack rows, one file at `~/.local/share/karaoke`.
class LibraryCache {
  public:
    LibraryCache();
    ~LibraryCache();

    LibraryCache(const LibraryCache &) = delete;
    LibraryCache &operator=(const LibraryCache &) = delete;

    /// False when SQLite or the data directory is unavailable.
    bool isOpen() const;

    /// Reads the row for `path`; false when there is none.
    bool lookup(const QString &path, CachedTrack *track) const;
    void save(const QString &path, const CachedTrack &track);
    /// Marks a row as still present, so the sweep at the end of a scan keeps it.
    void touch(const QString &path);

    /// Before a scan: the epoch rows are tagged with. After it: everything not
    /// touched by this scan is deleted, which is how removed files leave.
    void beginScan();
    void endScan();

  private:
    void close();

    QSqlDatabase m_db;
    int m_epoch = 0;
};

} // namespace library
