#include "playbackclock.h"

#include <QtMath>

PlaybackClock::PlaybackClock(QObject *parent)
    : QObject(parent)
{
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(16); // ~60 fps, plenty for a smooth sweep
    connect(&m_timer, &QTimer::timeout, this, &PlaybackClock::advance);
}

void PlaybackClock::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;

    m_playing = playing;
    if (m_playing) {
        m_base = m_position;
        m_elapsed.start();
        m_timer.start();
    } else {
        m_timer.stop();
        m_position = qMin(m_base + m_elapsed.elapsed() / 1000.0, m_duration > 0 ? m_duration
                                                                                : m_position);
        emit positionChanged();
    }
    emit playingChanged();
}

void PlaybackClock::setLoop(bool loop)
{
    if (m_loop == loop)
        return;
    m_loop = loop;
    emit loopChanged();
}

void PlaybackClock::setDuration(qreal duration)
{
    if (qFuzzyCompare(m_duration, duration))
        return;
    m_duration = duration;
    emit durationChanged();
}

void PlaybackClock::toggle()
{
    setPlaying(!m_playing);
}

void PlaybackClock::seek(qreal seconds)
{
    m_position = qMax<qreal>(0.0, seconds);
    if (m_duration > 0)
        m_position = qMin(m_position, m_duration);
    m_base = m_position;
    if (m_playing)
        m_elapsed.restart();
    emit positionChanged();
}

void PlaybackClock::advance()
{
    qreal position = m_base + m_elapsed.elapsed() / 1000.0;

    if (m_duration > 0 && position >= m_duration) {
        if (m_loop) {
            m_base = 0.0;
            m_elapsed.restart();
            position = 0.0;
        } else {
            position = m_duration;
            setPlaying(false);
        }
    }

    if (qFuzzyCompare(1.0 + position, 1.0 + m_position))
        return;
    m_position = position;
    emit positionChanged();
}
