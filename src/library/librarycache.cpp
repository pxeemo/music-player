#include "librarycache.h"

#include <QDir>
#include <QSqlQuery>
#include <QStandardPaths>

namespace library {

namespace {

// Bumped whenever the songs table changes shape: a database written by another
// schema is dropped and rebuilt from the files themselves.
constexpr int SchemaVersion = 1;

// One connection for the process; the cache lives as long as the library does.
constexpr const char *ConnectionName = "karaoke-library";

bool exec(QSqlDatabase &db, const QString &sql)
{
    QSqlQuery query(db);
    return query.exec(sql);
}

/// A null QString binds as SQL NULL. Every column here holds a value, with an
/// absent one spelled as the empty string - a sidecar that is not there is
/// `""`, not "no value", or the row would not write at all.
QVariant text(const QString &string)
{
    return string.isNull() ? QVariant(QString("")) : QVariant(string);
}

} // namespace

bool isFresh(const CachedTrack &track, qint64 mtimeMs, qint64 sizeBytes, qint64 sidecarMtimeMs)
{
    return track.mtimeMs == mtimeMs && track.sizeBytes == sizeBytes &&
           track.sidecarMtimeMs == sidecarMtimeMs;
}

LibraryCache::LibraryCache()
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (directory.isEmpty() || !QDir().mkpath(directory))
        return; // nowhere to keep a cache; callers scan without one

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QLatin1String(ConnectionName));
    m_db.setDatabaseName(directory + QStringLiteral("/library.db"));
    if (!m_db.open()) {
        close();
        return;
    }

    // A scan rewrites many rows in one go; WAL keeps that cheap and lets the
    // reader keep using the file while it happens.
    exec(m_db, QStringLiteral("PRAGMA journal_mode=WAL"));
    exec(m_db, QStringLiteral("PRAGMA synchronous=NORMAL"));

    int version = 0;
    {
        QSqlQuery query(m_db);
        if (query.exec(QStringLiteral("PRAGMA user_version")) && query.next())
            version = query.value(0).toInt();
    }
    if (version != SchemaVersion) {
        // Not migrated: the files on disk are the source of truth, and reading
        // them once is what any schema would have to reconstruct anyway.
        exec(m_db, QStringLiteral("DROP TABLE IF EXISTS songs"));
        exec(m_db, QStringLiteral("DROP TABLE IF EXISTS meta"));
    }

    const bool created = exec(m_db, QStringLiteral("CREATE TABLE IF NOT EXISTS songs ("
                                                   "  path TEXT PRIMARY KEY,"
                                                   "  mtime_ms INTEGER NOT NULL,"
                                                   "  size_bytes INTEGER NOT NULL,"
                                                   "  title TEXT NOT NULL,"
                                                   "  artist TEXT NOT NULL,"
                                                   "  album TEXT NOT NULL,"
                                                   "  duration REAL NOT NULL,"
                                                   "  artwork TEXT NOT NULL,"
                                                   "  added_ms INTEGER NOT NULL,"
                                                   "  sidecar TEXT NOT NULL,"
                                                   "  sidecar_mtime_ms INTEGER NOT NULL,"
                                                   "  lyrics BLOB,"
                                                   "  epoch INTEGER NOT NULL"
                                                   ")")) &&
                         exec(m_db, QStringLiteral("CREATE TABLE IF NOT EXISTS meta ("
                                                   "  key TEXT PRIMARY KEY,"
                                                   "  value TEXT NOT NULL"
                                                   ")"));
    if (!created) {
        close();
        return;
    }

    if (version != SchemaVersion) {
        QSqlQuery query(m_db);
        if (!query.exec(QStringLiteral("PRAGMA user_version = %1").arg(SchemaVersion)))
            close();
    }
}

LibraryCache::~LibraryCache()
{
    close();
}

bool LibraryCache::isOpen() const
{
    return m_db.isOpen();
}

bool LibraryCache::lookup(const QString &path, CachedTrack *track) const
{
    if (!m_db.isOpen() || !track)
        return false;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT mtime_ms, size_bytes, title, artist, album, duration, artwork, added_ms,"
        "       sidecar, sidecar_mtime_ms, lyrics"
        "  FROM songs WHERE path = ?"));
    query.addBindValue(path);
    if (!query.exec() || !query.next())
        return false;

    track->mtimeMs = query.value(0).toLongLong();
    track->sizeBytes = query.value(1).toLongLong();
    track->title = query.value(2).toString();
    track->artist = query.value(3).toString();
    track->album = query.value(4).toString();
    track->duration = query.value(5).toDouble();
    track->artwork = query.value(6).toString();
    track->addedMs = query.value(7).toLongLong();
    track->sidecar = query.value(8).toString();
    track->sidecarMtimeMs = query.value(9).toLongLong();
    track->lyrics = query.value(10).toByteArray();
    return true;
}

void LibraryCache::save(const QString &path, const CachedTrack &track)
{
    if (!m_db.isOpen())
        return;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO songs"
        " (path, mtime_ms, size_bytes, title, artist, album, duration, artwork, added_ms,"
        "  sidecar, sidecar_mtime_ms, lyrics, epoch)"
        " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(path);
    query.addBindValue(track.mtimeMs);
    query.addBindValue(track.sizeBytes);
    query.addBindValue(text(track.title));
    query.addBindValue(text(track.artist));
    query.addBindValue(text(track.album));
    query.addBindValue(track.duration);
    query.addBindValue(text(track.artwork));
    query.addBindValue(track.addedMs);
    query.addBindValue(text(track.sidecar));
    query.addBindValue(track.sidecarMtimeMs);
    query.addBindValue(track.lyrics);
    query.addBindValue(m_epoch);
    query.exec(); // a failed write costs the next scan the reuse, nothing more
}

void LibraryCache::touch(const QString &path)
{
    if (!m_db.isOpen())
        return;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE songs SET epoch = ? WHERE path = ?"));
    query.addBindValue(m_epoch);
    query.addBindValue(path);
    query.exec();
}

void LibraryCache::beginScan()
{
    m_epoch = 0;
    if (!m_db.isOpen())
        return;

    QSqlQuery query(m_db);
    if (query.exec(QStringLiteral("SELECT value FROM meta WHERE key = 'epoch'")) && query.next())
        m_epoch = query.value(0).toInt();

    ++m_epoch;
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO meta (key, value) VALUES ('epoch', ?)"));
    query.addBindValue(m_epoch);
    query.exec();
}

void LibraryCache::endScan()
{
    if (!m_db.isOpen() || m_epoch <= 0)
        return;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM songs WHERE epoch <> ?"));
    query.addBindValue(m_epoch);
    query.exec();
}

void LibraryCache::close()
{
    m_db.close();
    m_db = QSqlDatabase();
    // Only named connections are removed; there is none when the constructor
    // never got as far as opening one.
    if (QSqlDatabase::contains(QLatin1String(ConnectionName)))
        QSqlDatabase::removeDatabase(QLatin1String(ConnectionName));
}

} // namespace library
