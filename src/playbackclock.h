// playbackclock.h - a stand-in for an audio player.
//
// One continuously running position value drives every line in QML; there are
// no per-word timers. Swap this out for a real audio clock later by keeping
// the same four properties.
<<<<<<< HEAD
=======

>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

<<<<<<< HEAD

class PlaybackClock : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool loop READ loop WRITE setLoop NOTIFY loopChanged)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged)
    Q_PROPERTY(
        qreal duration READ duration WRITE setDuration NOTIFY durationChanged)

  public:
    explicit PlaybackClock(QObject *parent = nullptr);

    

=======
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

>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
    bool playing() const { return m_playing; }
    void setPlaying(bool playing);

    bool loop() const { return m_loop; }
    void setLoop(bool loop);

    qreal position() const { return m_position; }
    qreal duration() const { return m_duration; }
    void setDuration(qreal duration);

    Q_INVOKABLE void toggle();
    Q_INVOKABLE void seek(qreal seconds);

<<<<<<< HEAD
  signals:
=======
signals:
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
    void playingChanged();
    void loopChanged();
    void positionChanged();
    void durationChanged();

<<<<<<< HEAD
  private:
=======
private:
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
    void advance();

    QTimer m_timer;
    QElapsedTimer m_elapsed;
<<<<<<< HEAD
    qreal m_base = 0.0;
=======
    qreal m_base = 0.0;      // position at the moment playback was (re)started
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
    qreal m_position = 0.0;
    qreal m_duration = 0.0;
    bool m_playing = false;
    bool m_loop = true;
<<<<<<< HEAD
};
=======
};
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
