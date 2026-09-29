// song.h - a real track on disk plus the library that owns them.
//
// Song is pure data: the path, what the tags said, and the lyrics we managed to
// extract (sidecar .lrc/.ttml first, embedded tag otherwise). It deliberately
// keeps lyrics as a plain string for now; a real synced parser would replace
// this field, not the UI around it.
//
// MusicLibrary scans the user's music folder in the background (a few files per
// event loop tick) and appends Songs, so the list fills in without blocking the
// UI.

#pragma once

#include <QColor>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QtQml/qqmllist.h> // QQmlListProperty
#include <QtQml/qqmlregistration.h>

/// One track on disk.
class Song : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(QUrl source READ source NOTIFY filePathChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY artistChanged)
    Q_PROPERTY(QString album READ album NOTIFY albumChanged)
    Q_PROPERTY(qreal duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(QString lyrics READ lyrics NOTIFY lyricsChanged)
    Q_PROPERTY(bool hasLyrics READ hasLyrics NOTIFY lyricsChanged)
    Q_PROPERTY(QColor colorA READ colorA NOTIFY colorsChanged)
    Q_PROPERTY(QColor colorB READ colorB NOTIFY colorsChanged)

  public:
    explicit Song(QObject *parent = nullptr) : QObject(parent) {}

    QString filePath() const { return m_filePath; }
    QUrl source() const { return QUrl::fromLocalFile(m_filePath); }

    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    qreal duration() const { return m_duration; }
    QString lyrics() const { return m_lyrics; }
    bool hasLyrics() const { return !m_lyrics.isEmpty(); }

    // Deterministic cover colours derived from the track, so the artwork stays
    // stable across runs without needing embedded pictures.
    QColor colorA() const;
    QColor colorB() const;

    void setFilePath(const QString &path);
    void setTitle(const QString &title);
    void setArtist(const QString &artist);
    void setAlbum(const QString &album);
    void setDuration(qreal duration);
    void setLyrics(const QString &lyrics);

  signals:
    void filePathChanged();
    void titleChanged();
    void artistChanged();
    void albumChanged();
    void durationChanged();
    void lyricsChanged();
    void colorsChanged();

  private:
    QString m_filePath;
    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_lyrics;
    qreal m_duration = 0.0;
};

/// Scans the music folder and owns every Song it finds.
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

    QQmlListProperty<Song> songs() {
        return QQmlListProperty<Song>(this, &m_songs);
    }

    int count() const { return m_songs.size(); }
    bool scanning() const { return m_scanning; }
    int scanned() const { return m_scanned; }
    int total() const { return m_total; }
    QString folder() const { return m_folder; }

    const QList<Song *> &songList() const { return m_songs; }

    Q_INVOKABLE void scan();
    Q_INVOKABLE Song *songAt(int index) const;

  signals:
    void songsChanged();
    void scanningChanged();
    void progressChanged();
    void folderChanged();

  private:
    void startScan();
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
