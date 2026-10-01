#include "library.h"

#include "trackloader.h"

#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>

#include <algorithm>

namespace {

int compare(const QDateTime &left, const QDateTime &right)
{
    if (left < right)
        return -1;
    if (right < left)
        return 1;
    return 0;
}

int compare(const QString &left, const QString &right)
{
    return left.compare(right, Qt::CaseInsensitive);
}

// Order two songs for a sort option. Positive means `a` sorts after `b`.
int compareSongs(const Song *a, const Song *b, MusicLibrary::SortOrder order)
{
    switch (order) {
    case MusicLibrary::ModifiedDate:
        return compare(b->modifiedTime(), a->modifiedTime()); // newest first
    case MusicLibrary::ModifiedDateReverse:
        return compare(a->modifiedTime(), b->modifiedTime());
    case MusicLibrary::AddedDate:
        return compare(b->addedTime(), a->addedTime()); // newest first
    case MusicLibrary::AddedDateReverse:
        return compare(a->addedTime(), b->addedTime());
    case MusicLibrary::Title:
        return compare(a->title(), b->title());
    case MusicLibrary::TitleReverse:
        return compare(b->title(), a->title());
    case MusicLibrary::Artist:
        return compare(a->artist(), b->artist());
    case MusicLibrary::ArtistReverse:
        return compare(b->artist(), a->artist());
    case MusicLibrary::Album:
        return compare(a->album(), b->album());
    case MusicLibrary::AlbumReverse:
        return compare(b->album(), a->album());
    }
    return 0;
}

bool matchesFilter(const Song *song, const QString &filter)
{
    return song->title().contains(filter, Qt::CaseInsensitive) ||
           song->artist().contains(filter, Qt::CaseInsensitive) ||
           song->album().contains(filter, Qt::CaseInsensitive);
}

} // namespace

MusicLibrary *MusicLibrary::s_instance = nullptr;

MusicLibrary::MusicLibrary(QObject *parent) : QObject(parent)
{
    s_instance = this;

    m_timer.setInterval(0);
    connect(&m_timer, &QTimer::timeout, this, &MusicLibrary::processChunk);
}

void MusicLibrary::setSortOrder(SortOrder order)
{
    if (m_sortOrder == order)
        return;
    m_sortOrder = order;
    emit sortOrderChanged();
    rebuildView();
}

void MusicLibrary::setFilterText(const QString &text)
{
    if (m_filter == text)
        return;
    m_filter = text;
    emit filterTextChanged();
    rebuildView();
}

void MusicLibrary::scan()
{
    if (m_scanning)
        return;

    qDeleteAll(m_songs);
    m_songs.clear();
    m_view.clear();
    m_pending.clear();
    m_scanned = 0;
    emit songsChanged();

    QString folder = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    if (folder.isEmpty() || !QDir(folder).exists()) {
        const QString home = QDir::homePath();
        for (const QString &candidate : {home + "/mus", home + "/Music"}) {
            if (QDir(candidate).exists()) {
                folder = candidate;
                break;
            }
        }
    }
    m_folder = folder;
    emit folderChanged();

    if (folder.isEmpty()) {
        m_total = 0;
        emit progressChanged();
        return;
    }

    QDirIterator it(folder, library::audioNameFilters(), QDir::Files | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    while (it.hasNext())
        m_pending.append(it.next());

    m_total = m_pending.size();
    m_scanning = true;
    emit progressChanged();
    emit scanningChanged();

    if (m_pending.isEmpty()) {
        m_scanning = false;
        emit scanningChanged();
        return;
    }
    m_timer.start();
}

void MusicLibrary::processChunk()
{
    // A few files per tick keeps the UI responsive while the list fills in.
    constexpr int chunkSize = 24;
    int processed = 0;
    while (!m_pending.isEmpty() && processed < chunkSize) {
        const QString path = m_pending.takeFirst();
        auto *song = new Song(this);
        library::loadTrack(path, song);
        m_songs.append(song);
        ++m_scanned;
        ++processed;
    }

    emit progressChanged();

    const bool finished = m_pending.isEmpty();
    if (finished || m_scanned % 240 == 0) {
        // Publish periodically so the list grows while scanning, without
        // rebuilding it once per file.
        rebuildView();
    }

    if (finished) {
        m_timer.stop();
        m_scanning = false;
        emit scanningChanged();
    }
}

void MusicLibrary::rebuildView()
{
    const QString filter = m_filter.trimmed();
    QList<Song *> view;
    view.reserve(m_songs.size());
    for (Song *song : m_songs) {
        if (filter.isEmpty() || matchesFilter(song, filter))
            view.append(song);
    }

    const SortOrder order = m_sortOrder;
    std::stable_sort(view.begin(), view.end(), [order](const Song *a, const Song *b) {
        const int primary = compareSongs(a, b, order);
        if (primary != 0)
            return primary < 0;
        // Stable, predictable tie-break.
        const int byTitle = compare(a->title(), b->title());
        if (byTitle != 0)
            return byTitle < 0;
        return a->filePath() < b->filePath();
    });

    m_view = view;
    emit songsChanged();
}
