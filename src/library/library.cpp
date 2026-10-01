#include "library.h"

#include "trackloader.h"

#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>

MusicLibrary *MusicLibrary::s_instance = nullptr;

MusicLibrary::MusicLibrary(QObject *parent) : QObject(parent)
{
    s_instance = this;

    m_timer.setInterval(0);
    connect(&m_timer, &QTimer::timeout, this, &MusicLibrary::processChunk);
}

void MusicLibrary::scan()
{
    if (m_scanning)
        return;

    qDeleteAll(m_songs);
    m_songs.clear();
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

    if (m_pending.isEmpty()) {
        m_timer.stop();
        m_scanning = false;
        emit songsChanged();
        emit scanningChanged();
    } else if (m_scanned % 240 == 0) {
        // Publish periodically so the list grows while scanning, without
        // rebuilding it once per file.
        emit songsChanged();
    }
}
