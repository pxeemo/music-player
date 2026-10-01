// player.h - real audio playback on top of GStreamer.
//
// One `playbin` does the decoding, driven from the Qt event loop: a timer polls
// the GStreamer bus for errors/end-of-stream and queries the play position. No
// GLib main loop is needed, so the player drops straight into a Qt app.
//
// The player also owns the queue. Repeat and shuffle are decided here, so the
// QML just shows their state.

#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmllist.h> // QQmlListProperty
#include <QtQml/qqmlregistration.h>

#include "library/song.h"

// Opaque GStreamer pointers, so the header does not need the GStreamer headers.
typedef struct _GstElement GstElement;
typedef struct _GstBus GstBus;

class Player : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged)
    Q_PROPERTY(qreal duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(Song *currentSong READ currentSong NOTIFY currentSongChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QQmlListProperty<Song> queue READ queue NOTIFY queueChanged)
    Q_PROPERTY(int queueCount READ queueCount NOTIFY queueChanged)
    Q_PROPERTY(RepeatMode repeatMode READ repeatMode NOTIFY repeatModeChanged)
    Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY shuffleChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

  public:
    enum RepeatMode {
        RepeatOff = 0,
        RepeatAll = 1,
        RepeatOne = 2,
    };
    Q_ENUM(RepeatMode)

    explicit Player(QObject *parent = nullptr);
    ~Player() override;

    bool playing() const { return m_playing; }
    qreal position() const { return m_position; }
    qreal duration() const { return m_duration; }
    qreal volume() const { return m_volume; }
    Song *currentSong() const;
    int currentIndex() const { return m_index; }
    int queueCount() const { return m_queue.size(); }
    QList<Song *> &queueList() { return m_queue; }
    QQmlListProperty<Song> queue() { return QQmlListProperty<Song>(this, &m_queue); }
    RepeatMode repeatMode() const { return m_repeat; }
    bool shuffle() const { return m_shuffle; }
    QString errorString() const { return m_error; }

    void setVolume(qreal volume);
    void setShuffle(bool shuffle);

    /// Replaces the queue with the whole library and starts at `libraryIndex`.
    Q_INVOKABLE void playFromLibrary(int libraryIndex);
    /// Plays an entry of the current queue.
    Q_INVOKABLE void playQueueIndex(int queueIndex);
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void seek(qreal seconds);
    Q_INVOKABLE void cycleRepeat();
    Q_INVOKABLE void toggleShuffle();

  signals:
    void playingChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void currentSongChanged();
    void currentIndexChanged();
    void queueChanged();
    void repeatModeChanged();
    void shuffleChanged();
    void errorStringChanged();

  private:
    void setPlaying(bool playing);
    void playAt(int queueIndex);
    void loadCurrent();
    void stopInternal();
    void advance(bool automatic);
    void shuffleQueue();
    void poll();
    void updatePosition();
    void updateDuration();

    GstElement *m_playbin = nullptr;
    GstBus *m_bus = nullptr;
    QTimer m_pollTimer;

    QList<Song *> m_queue;         // the order actually played
    QList<Song *> m_originalQueue; // the unshuffled order, for when shuffle is off
    int m_index = -1;
    RepeatMode m_repeat = RepeatOff;
    bool m_shuffle = false;
    bool m_playing = false;
    qreal m_position = 0.0;
    qreal m_duration = 0.0;
    qreal m_volume = 1.0;
    QString m_error;
};
