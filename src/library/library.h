// library.h - scans the music folder and owns every Song it finds.
//
// The master list is kept in scan order; `songs` (and `songList`) expose a
// filtered, sorted view over it, so the UI and the play queue always agree on
// the visible order. Scanning runs off the event loop a chunk at a time.
// Reading a single file's tags/cover/lyrics is library::loadTrack()'s job.
//
// A scan does not re-read what it has already read: loadTrack() is handed the
// SQLite cache, which returns a file's row while the file is still the one that
// row was written from, and beginScan()/endScan() drop the rows of files that
// have gone away.

#pragma once

#include "librarycache.h"
#include "song.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QtQml/qqmllist.h> // QQmlListProperty
#include <QtQml/qqmlregistration.h>

class MusicLibrary : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QQmlListProperty<Song> songs READ songs NOTIFY songsChanged)
    Q_PROPERTY(int count READ count NOTIFY songsChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(int scanned READ scanned NOTIFY progressChanged)
    Q_PROPERTY(int total READ total NOTIFY progressChanged)
    Q_PROPERTY(QString folder READ folder NOTIFY folderChanged)
    Q_PROPERTY(SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)

  public:
    /// Sort options and their reverses. Date orders list newest first; text
    /// orders are A-Z; the `...Reverse` values flip that.
    enum SortOrder {
        ModifiedDate,
        ModifiedDateReverse,
        AddedDate,
        AddedDateReverse,
        Title,
        TitleReverse,
        Artist,
        ArtistReverse,
        Album,
        AlbumReverse,
    };
    Q_ENUM(SortOrder)

    explicit MusicLibrary(QObject *parent = nullptr);

    /// The single QML singleton instance, for C++ code that needs it.
    static MusicLibrary *instance() { return s_instance; }

    QQmlListProperty<Song> songs() { return QQmlListProperty<Song>(this, &m_view); }
    int count() const { return m_view.size(); }
    bool scanning() const { return m_scanning; }
    int scanned() const { return m_scanned; }
    int total() const { return m_total; }
    QString folder() const { return m_folder; }

    SortOrder sortOrder() const { return m_sortOrder; }
    void setSortOrder(SortOrder order);

    QString filterText() const { return m_filter; }
    void setFilterText(const QString &text);

    /// The filtered, sorted list. The play queue is built from this.
    const QList<Song *> &songList() const { return m_view; }

    /// Clears the library and rescans the music folder.
    Q_INVOKABLE void scan();

  signals:
    void songsChanged();
    void scanningChanged();
    void progressChanged();
    void folderChanged();
    void sortOrderChanged();
    void filterTextChanged();

  private:
    void processChunk();
    void rebuildView();
    /// Re-reads a song whose cached lyrics did not read back (see
    /// `Song::lyricsSourceBroken`), replacing the broken row.
    void refreshSong();

    static MusicLibrary *s_instance;

    QList<Song *> m_songs; // master list, in scan order
    QList<Song *> m_view;  // filtered + sorted
    QString m_filter;
    SortOrder m_sortOrder = ModifiedDate;
    QStringList m_pending;
    QString m_folder;
    QTimer m_timer;
    library::LibraryCache m_cache; // rows still valid from the last scan
    int m_scanned = 0;
    int m_total = 0;
    bool m_scanning = false;
};
