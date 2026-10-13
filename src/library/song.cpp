#include "song.h"

#include "lyrics/lyricsserializer.h"

#include <utility>

Song::Song(QObject *parent) : QObject(parent) {}

void Song::applyPending()
{
    if (!m_document)
        m_document = new LyricsDocument(this);

    if (!m_pendingJson.isEmpty()) {
        const QByteArray json = std::move(m_pendingJson);
        m_pendingJson = QByteArray();
        if (std::optional<lyrics::Lyrics> parsed = lyrics::fromJson(json))
            m_document->setLyrics(std::move(*parsed));
        else
            emit lyricsSourceBroken(); // the library re-reads the file
        return;
    }
    if (m_pendingFromLyrics) {
        m_pendingFromLyrics = false;
        m_document->setLyrics(std::move(m_pendingLyrics));
    }
}

LyricsDocument *Song::document()
{
    applyPending();
    return m_document;
}

void Song::setLyricsDocument(lyrics::Lyrics &&document)
{
    m_pendingJson = QByteArray();
    m_pendingLyrics = std::move(document);
    m_pendingFromLyrics = true;
    if (m_document)
        applyPending(); // already showing: publish the new rows at once
}

void Song::setLyricsJson(const QByteArray &json)
{
    m_pendingJson = json;
    m_pendingFromLyrics = false;
    if (m_document)
        applyPending();
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
