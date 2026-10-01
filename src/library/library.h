// library.h - scans the music folder and owns every Song it finds.
//
// Scanning runs off the event loop a chunk at a time, so the songs list fills in
// without blocking the UI. Reading a single file's tags/cover/lyrics is
// library::loadTrack()'s job.

#pragma once

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

  public:
    explicit MusicLibrary(QObject *parent = nullptr);

    /// The single QML singleton instance, for C++ code that needs it.
    static MusicLibrary *instance() { return s_instance; }

    QQmlListProperty<Song> songs() { return QQmlListProperty<Song>(this, &m_songs); }
    int count() const { return m_songs.size(); }
    bool scanning() const { return m_scanning; }
    int scanned() const { return m_scanned; }
    int total() const { return m_total; }
    QString folder() const { return m_folder; }

    const QList<Song *> &songList() const { return m_songs; }

    /// Clears the library and rescans the music folder.
    Q_INVOKABLE void scan();

  signals:
    void songsChanged();
    void scanningChanged();
    void progressChanged();
    void folderChanged();

  private:
    void processChunk();

    static MusicLibrary *s_instance;

    QList<Song *> m_songs;
    QStringList m_pending;
    QString m_folder;
    QTimer m_timer;
    int m_scanned = 0;
    int m_total = 0;
    bool m_scanning = false;
};
