// song.h - one track and everything known about it.
//
// Song is the data the UI binds to: the file, the tags, a cached cover image and
// the parsed lyrics. Filling it in is the job of library::loadTrack().

#pragma once

#include "lyrics/lyrics.h"
#include "lyrics/lyricsdocument.h"

#include <QColor>
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class Song : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(QUrl source READ source NOTIFY filePathChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY artistChanged)
    Q_PROPERTY(QString album READ album NOTIFY albumChanged)
    Q_PROPERTY(qreal duration READ duration NOTIFY durationChanged)
    /// Cached cover image, or an empty URL when the track has no embedded art.
    Q_PROPERTY(QUrl artworkSource READ artworkSource NOTIFY artworkChanged)
    Q_PROPERTY(QColor colorA READ colorA NOTIFY colorsChanged)
    Q_PROPERTY(QColor colorB READ colorB NOTIFY colorsChanged)
    /// The structured lyrics, already flattened for the view. Never null.
    Q_PROPERTY(LyricsDocument *document READ document CONSTANT)

  public:
    explicit Song(QObject *parent = nullptr);

    QString filePath() const { return m_filePath; }
    QUrl source() const { return QUrl::fromLocalFile(m_filePath); }

    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    qreal duration() const { return m_duration; }
    QUrl artworkSource() const { return m_artworkSource; }

    /// File timestamps, used by the library's sorting options.
    QDateTime modifiedTime() const { return m_modifiedTime; }
    QDateTime addedTime() const { return m_addedTime; }

    LyricsDocument *document() const { return m_document; }

    // Deterministic cover colours derived from the track, so the fallback
    // artwork stays stable without needing an embedded picture.
    QColor colorA() const;
    QColor colorB() const;

    void setFilePath(const QString &path);
    void setTitle(const QString &title);
    void setArtist(const QString &artist);
    void setAlbum(const QString &album);
    void setDuration(qreal duration);
    void setArtworkSource(const QUrl &source);
    void setModifiedTime(const QDateTime &time) { m_modifiedTime = time; }
    void setAddedTime(const QDateTime &time) { m_addedTime = time; }
    /// Hands a parsed document to the UI.
    void setLyricsDocument(lyrics::Lyrics &&document);

  signals:
    void filePathChanged();
    void titleChanged();
    void artistChanged();
    void albumChanged();
    void durationChanged();
    void artworkChanged();
    void colorsChanged();

  private:
    QString m_filePath;
    QString m_title;
    QString m_artist;
    QString m_album;
    qreal m_duration = 0.0;
    QUrl m_artworkSource;
    QDateTime m_modifiedTime;
    QDateTime m_addedTime;
    LyricsDocument *m_document = nullptr;
};
