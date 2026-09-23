// playbackclock.h - a stand-in for an audio player.
//
// One continuously running position value drives every line in QML; there are
// no per-word timers. Swap this out for a real audio clock later by keeping
// the same four properties.

#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class PlaybackClock : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool loop READ loop WRITE setLoop NOTIFY loopChanged)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged)
    Q_PROPERTY(qreal duration READ duration WRITE setDuration NOTIFY durationChanged)
    QML_ELEMENT

public:
    explicit PlaybackClock(QObject *parent = nullptr);

    bool playing() const { return m_playing; }
    void setPlaying(bool playing);

    bool loop() const { return m_loop; }
    void setLoop(bool loop);

    qreal position() const { return m_position; }
    qreal duration() const { return m_duration; }
    void setDuration(qreal duration);

    Q_INVOKABLE void toggle();
    Q_INVOKABLE void seek(qreal seconds);

signals:
    void playingChanged();
    void loopChanged();
    void positionChanged();
    void durationChanged();

private:
    void advance();

    QTimer m_timer;
    QElapsedTimer m_elapsed;
    qreal m_base = 0.0;      // position at the moment playback was (re)started
    qreal m_position = 0.0;
    qreal m_duration = 0.0;
    bool m_playing = false;
    bool m_loop = true;
};
