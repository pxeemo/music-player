// lyricsdocument.h - the QML-facing view of a structured `lyrics::Lyrics`.
//
// The model in lyricsmodel.h is deliberately plain C++: no QObject, no QML.
// This file adapts it for the UI. It flattens the ordered element list into a
// row per thing the view draws (a main line, its backgrounds, its translations,
// a section, an instrumental) and exposes the handful of facts a renderer
// needs: the text, what kind of row it is, the agent, and the timing used both
// for "is this the current line?" and for click-to-seek.
//
// Rows are immutable and rebuilt whenever a parser hands over a new document,
// so QML only ever reads. Nothing here knows how to parse anything.
//
// A `LyricsDocument` owns the `lyrics::Lyrics` it was given, so `Song` can keep
// one per track and replace its contents when the parser runs.

#pragma once

#include "lyricsmodel.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QtQml/qqmllist.h> // QQmlListProperty
#include <QtQml/qqmlregistration.h>

/// One drawn row of a lyrics document.
class LyricRow : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by LyricsDocument; not created from QML.")

    Q_PROPERTY(QString text READ text CONSTANT)
    Q_PROPERTY(QString agentName READ agentName CONSTANT)
    Q_PROPERTY(bool isMain READ isMain CONSTANT)
    Q_PROPERTY(bool isBackground READ isBackground CONSTANT)
    Q_PROPERTY(bool isTranslation READ isTranslation CONSTANT)
    Q_PROPERTY(bool isSection READ isSection CONSTANT)
    Q_PROPERTY(bool isInstrumental READ isInstrumental CONSTANT)
    /// Where a click should seek to, in ms, or -1 when the row is untimed.
    Q_PROPERTY(qint64 startMs READ startMs CONSTANT)
    /// Rows that belong to the same line share a group index.
    Q_PROPERTY(int groupIndex READ groupIndex CONSTANT)
    /// True for the first row of a group, so the view can space groups apart.
    Q_PROPERTY(bool groupStart READ groupStart CONSTANT)

  public:
    enum Kind {
        Main,
        Background,
        Translation,
        Section,
        Instrumental,
    };
    Q_ENUM(Kind)

    LyricRow(Kind kind, QString text, QString agentName, qint64 startMs, qint64 groupStartMs,
             int groupIndex, bool groupStart, QObject *parent = nullptr);

    Kind kind() const { return m_kind; }
    QString text() const { return m_text; }
    QString agentName() const { return m_agentName; }

    bool isMain() const { return m_kind == Main; }
    bool isBackground() const { return m_kind == Background; }
    bool isTranslation() const { return m_kind == Translation; }
    bool isSection() const { return m_kind == Section; }
    bool isInstrumental() const { return m_kind == Instrumental; }

    qint64 startMs() const { return m_startMs; }
    int groupIndex() const { return m_groupIndex; }
    bool groupStart() const { return m_groupStart; }
    /// Start of the whole group, used to pick the active line; -1 when untimed.
    qint64 groupStartMs() const { return m_groupStartMs; }

  private:
    Kind m_kind;
    QString m_text;
    QString m_agentName;
    qint64 m_startMs;
    qint64 m_groupStartMs;
    int m_groupIndex;
    bool m_groupStart;
};

/// The flattened, UI-ready form of a `lyrics::Lyrics`.
class LyricsDocument : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQmlListProperty<LyricRow> rows READ rows NOTIFY rowsChanged)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowsChanged)
    Q_PROPERTY(bool empty READ empty NOTIFY rowsChanged)
    /// True when at least one row carries timing (so the view may highlight).
    Q_PROPERTY(bool timed READ timed NOTIFY rowsChanged)
    /// Human-readable dump of the structured model, for debugging a parser.
    Q_PROPERTY(QString debugText READ debugText NOTIFY rowsChanged)

  public:
    explicit LyricsDocument(QObject *parent = nullptr);
    ~LyricsDocument() override;

    /// Takes ownership of `document` and rebuilds the rows.
    void setLyrics(lyrics::Lyrics &&document);

    bool hasLyrics() const { return m_hasLyrics; }
    QQmlListProperty<LyricRow> rows() { return QQmlListProperty<LyricRow>(this, &m_rows); }
    int rowCount() const { return m_rows.size(); }
    bool empty() const { return m_rows.isEmpty(); }
    bool timed() const { return m_timed; }
    QString debugText() const;

    Q_INVOKABLE LyricRow *rowAt(int index) const;
    /// Group index of the last group that has started at `positionMs`, or -1.
    Q_INVOKABLE int activeGroupIndex(qint64 positionMs) const;
    /// Row index of a group's first row, or -1.
    Q_INVOKABLE int firstRowOfGroup(int groupIndex) const;

  signals:
    void rowsChanged();

  private:
    void rebuild();
    LyricRow *addRow(LyricRow::Kind kind, const QString &text, const QString &agentName,
                     qint64 startMs, qint64 groupStartMs, int groupIndex, bool groupStart);
    QString agentName(lyrics::Id id) const;

    lyrics::Lyrics m_lyrics;
    bool m_hasLyrics = false;
    bool m_timed = false;
    QList<LyricRow *> m_rows;
};
