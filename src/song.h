// song.h - the song catalogue and its lyrics.
//
// A Song is the unit the UI navigates: a title, an artist, a pair of colours
// used to paint its artwork and the lyric lines synced to the playback clock.
// SongsModel owns every Song and is exposed to QML as a singleton, the same way
// PlaybackClock is: the songs list page is simply a view over its `songs`.

#pragma once

#include "lyrics.h"

#include <QColor>
#include <QObject>
#include <QString>
#include <QtQml/qqmllist.h> // QQmlListProperty
#include <QtQml/qqmlregistration.h>

/// One song: metadata, a gradient used as its cover, and its lyric lines.
class Song : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist WRITE setArtist NOTIFY artistChanged)
    Q_PROPERTY(QColor colorA READ colorA WRITE setColorA NOTIFY colorAChanged)
    Q_PROPERTY(QColor colorB READ colorB WRITE setColorB NOTIFY colorBChanged)
    Q_PROPERTY(QQmlListProperty<LyricLine> lyrics READ lyrics NOTIFY lyricsChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY lyricsChanged)
    Q_PROPERTY(qreal duration READ duration NOTIFY lyricsChanged)
    QML_ELEMENT

  public:
    explicit Song(QObject *parent = nullptr) : QObject(parent) {}

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString artist() const { return m_artist; }
    void setArtist(const QString &artist);

    QColor colorA() const { return m_colorA; }
    void setColorA(const QColor &color);

    QColor colorB() const { return m_colorB; }
    void setColorB(const QColor &color);

    QQmlListProperty<LyricLine> lyrics() {
        return QQmlListProperty<LyricLine>(this, &m_lyrics);
    }

    int lineCount() const { return m_lyrics.size(); }
    qreal duration() const;

    Q_INVOKABLE LyricLine *lineAt(int index) const;
    Q_INVOKABLE LyricLine *addLine();
    Q_INVOKABLE void clear();
    Q_INVOKABLE int activeLine(qreal position) const;

    // Loaders wrap a batch of addLine()/addWord() calls between these so the
    // model announces itself once instead of once per word.
    void beginLoad();
    void endLoad();

  signals:
    void titleChanged();
    void artistChanged();
    void colorAChanged();
    void colorBChanged();
    void lyricsChanged();

  private:
    void notifyLyricsChanged();

    QString m_title;
    QString m_artist;
    QColor m_colorA = QColor("#3a3a55");
    QColor m_colorB = QColor("#141420");
    QList<LyricLine *> m_lyrics;
    bool m_bulkLoading = false;
};

/// Owns every Song in the library and builds the hard-coded demo catalogue.
class SongsModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QQmlListProperty<Song> songs READ songs NOTIFY songsChanged)
    Q_PROPERTY(int count READ count NOTIFY songsChanged)

  public:
    explicit SongsModel(QObject *parent = nullptr) : QObject(parent) {}

    QQmlListProperty<Song> songs() {
        return QQmlListProperty<Song>(this, &m_songs);
    }

    int count() const { return m_songs.size(); }

    Q_INVOKABLE Song *songAt(int index) const;
    Q_INVOKABLE Song *addSong();
    Q_INVOKABLE void clear();
    Q_INVOKABLE void loadDemoSongs();

  signals:
    void songsChanged();

  private:
    QList<Song *> m_songs;
};
