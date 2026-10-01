#include "song.h"

#include <utility>

Song::Song(QObject *parent) : QObject(parent), m_document(new LyricsDocument(this)) {}

void Song::setLyricsDocument(lyrics::Lyrics &&document)
{
    m_document->setLyrics(std::move(document));
}

void Song::setFilePath(const QString &path)
{
    if (m_filePath == path)
        return;
    m_filePath = path;
    emit filePathChanged();
}

void Song::setTitle(const QString &title)
{
    if (m_title == title)
        return;
    m_title = title;
    emit titleChanged();
    emit colorsChanged();
}

void Song::setArtist(const QString &artist)
{
    if (m_artist == artist)
        return;
    m_artist = artist;
    emit artistChanged();
    emit colorsChanged();
}

void Song::setAlbum(const QString &album)
{
    if (m_album == album)
        return;
    m_album = album;
    emit albumChanged();
}

void Song::setDuration(qreal duration)
{
    if (qFuzzyCompare(m_duration, duration))
        return;
    m_duration = duration;
    emit durationChanged();
}

void Song::setArtworkSource(const QUrl &source)
{
    if (m_artworkSource == source)
        return;
    m_artworkSource = source;
    emit artworkChanged();
}

QColor Song::colorA() const
{
    const uint hash = qHash(m_title + m_artist);
    return QColor::fromHsv(int(hash % 360), 150, 225);
}

QColor Song::colorB() const
{
    const uint hash = qHash(m_artist + m_title);
    return QColor::fromHsv(int((hash / 360 + 45) % 360), 190, 120);
}
