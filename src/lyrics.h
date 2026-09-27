// lyrics.h - the timing model of the demo.
//
// These types deliberately know nothing about shaders, Text items or playback
// UI: they only carry text plus word start/end timestamps. A later phase can
// replace the demo loaders with an LRC/TTML parser that fills the exact same
// structure, and the rendering layer will not have to change.

#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmllist.h> // QQmlListProperty
#include <QtQml/qqmlregistration.h>

/// One sung word with its time range, in seconds from the start of the song.
class Word : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(qreal start READ start WRITE setStart NOTIFY startChanged)
    Q_PROPERTY(qreal end READ end WRITE setEnd NOTIFY endChanged)
    QML_ELEMENT

  public:
    explicit Word(QObject *parent = nullptr) : QObject(parent) {}

    QString text() const { return m_text; }
    void setText(const QString &text) {
        if (m_text == text)
            return;
        m_text = text;
        emit textChanged();
    }

    qreal start() const { return m_start; }
    void setStart(qreal start) {
        if (qFuzzyCompare(m_start, start))
            return;
        m_start = start;
        emit startChanged();
    }

    qreal end() const { return m_end; }
    void setEnd(qreal end) {
        if (qFuzzyCompare(m_end, end))
            return;
        m_end = end;
        emit endChanged();
    }

  signals:
    void textChanged();
    void startChanged();
    void endChanged();

  private:
    QString m_text;
    qreal m_start = 0.0;
    qreal m_end = 0.0;
};

/// One lyric line: an ordered list of words. start/end follow the first and
/// last word, so no separate line timing has to be kept in sync.
class LyricLine : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<Word> words READ words NOTIFY wordsChanged)
    Q_PROPERTY(QString text READ text NOTIFY wordsChanged)
    Q_PROPERTY(qreal start READ start NOTIFY wordsChanged)
    Q_PROPERTY(qreal end READ end NOTIFY wordsChanged)
    QML_ELEMENT

  public:
    explicit LyricLine(QObject *parent = nullptr) : QObject(parent) {}

    QQmlListProperty<Word> words() {
        return QQmlListProperty<Word>(this, &m_words);
    }

    QString text() const;
    qreal start() const;
    qreal end() const;

    Q_INVOKABLE Word *addWord(const QString &text, qreal start, qreal end);
    Q_INVOKABLE void clear();

  signals:
    void wordsChanged();

  private:
    QList<Word *> m_words;
};
