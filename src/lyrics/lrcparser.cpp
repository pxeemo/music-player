#include "lrcparser.h"

#include <QRegularExpression>
#include <QStringList>
#include <QVector>

#include <chrono>
#include <utility>

namespace lyrics {

namespace {

using Milliseconds = std::chrono::milliseconds;

// "mm:ss", "mm:ss.xx" or "mm:ss:xx". Partial seconds are scaled by their digit
// count, so ".5", ".50" and ".500" all mean 500 ms.
bool parseTimestamp(const QString &value, qint64 *out)
{
    static const QRegularExpression re(
        QStringLiteral("^\\s*(\\d{1,3}):(\\d{1,2})(?:[.:](\\d{1,3}))?\\s*$"));
    const QRegularExpressionMatch match = re.match(value);
    if (!match.hasMatch())
        return false;

    const qint64 minutes = match.captured(1).toLongLong();
    const qint64 seconds = match.captured(2).toLongLong();
    const QString fraction = match.captured(3);
    qint64 millis = 0;
    if (!fraction.isEmpty()) {
        if (fraction.size() == 1)
            millis = fraction.toLongLong() * 100;
        else if (fraction.size() == 2)
            millis = fraction.toLongLong() * 10;
        else
            millis = fraction.toLongLong();
    }
    *out = (minutes * 60 + seconds) * 1000 + millis;
    return true;
}

// Removes enhanced-LRC `<mm:ss.xx>` tags so the line reads as plain text.
QString stripWordTags(QString text)
{
    static const QRegularExpression tag(
        QStringLiteral("<\\s*\\d{1,3}:\\d{1,2}(?:[.:]\\d{1,3})?\\s*>"));
    text.remove(tag);
    static const QRegularExpression spaces(QStringLiteral("[ \\t]+"));
    text.replace(spaces, QStringLiteral(" "));
    return text.trimmed();
}

// Splits enhanced-LRC text into words whose starts come from `<mm:ss.xx>` tags.
// A tag also closes the previous word and extends the line's end. Returns the
// (possibly updated) line end, in milliseconds.
qint64 buildWords(Line &line, const QString &text, qint64 lineStart, qint64 lineEnd, qint64 offset)
{
    static const QRegularExpression token(
        QStringLiteral("<\\s*\\d{1,3}:\\d{1,2}(?:[.:]\\d{1,3})?\\s*>|[^\\s<]+"));
    static const QRegularExpression tag(QStringLiteral("^<\\s*(.*?)\\s*>$"));

    struct WordTiming {
        QString text;
        qint64 start;
        qint64 end;
    };

    QVector<WordTiming> words;
    qint64 timestamp = -1;

    QRegularExpressionMatchIterator matches = token.globalMatch(text);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        const QString tokenText = match.captured(0);

        if (tokenText.startsWith(QLatin1Char('<'))) {
            const QRegularExpressionMatch tagMatch = tag.match(tokenText);
            qint64 parsed = 0;
            if (tagMatch.hasMatch() && parseTimestamp(tagMatch.captured(1), &parsed)) {
                timestamp = qMax<qint64>(0, parsed + offset);
                lineEnd = timestamp;
                if (!words.isEmpty() && words.back().end == 0)
                    words.back().end = timestamp;
            }
            continue;
        }

        if (timestamp < 0)
            continue;

        words.append({.text = tokenText, .start = timestamp, .end = 0});
        timestamp = -1;
    }

    if (words.isEmpty())
        return lineEnd;

    qint64 cursor = lineStart;
    line.mainVocal.words.reserve(line.mainVocal.words.size() + words.size());
    for (WordTiming &info : words) {
        info.start = qMax(info.start, cursor);
        info.end = qMax(info.end, info.start);

        Word word;
        word.text = info.text;
        word.timing = Timing{Milliseconds(info.start), Milliseconds(info.end)};
        line.mainVocal.words.push_back(std::move(word));

        cursor = info.end;
    }
    return lineEnd;
}

} // namespace

QString LrcParser::formatName() const
{
    return QStringLiteral("lrc");
}

bool LrcParser::looksLikeLrc(const QString &text)
{
    static const QRegularExpression re(
        QStringLiteral("\\[\\s*\\d{1,3}:\\d{1,2}(?:[.:]\\d{1,3})?\\s*\\]"));
    return re.match(text).hasMatch();
}

ParseResult LrcParser::parse(const QByteArray &data) const
{
    if (data.isEmpty())
        return {std::nullopt, QStringLiteral("empty input")};

    Lyrics document;
    const QString content = QString::fromUtf8(data);

    struct Pending {
        qint64 start; // -1 when the line has no timestamp
        QString text;
    };
    QVector<Pending> pending;
    qint64 offset = 0;
    int timedLines = 0;

    const QStringList rawLines = content.split(QLatin1Char('\n'));
    for (QString line : rawLines) {
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        line = line.trimmed();
        if (line.isEmpty())
            continue;

        QVector<qint64> times;
        QString text = line;

        // Consume every leading `[...]` tag: timestamps and metadata alike.
        while (text.startsWith(QLatin1Char('['))) {
            const int close = text.indexOf(QLatin1Char(']'));
            if (close < 0)
                break;
            const QString tag = text.mid(1, close - 1).trimmed();
            text = text.mid(close + 1);
            if (tag.isEmpty())
                continue;

            qint64 ms = 0;
            if (parseTimestamp(tag, &ms)) {
                times.append(ms);
                continue;
            }
            const int colon = tag.indexOf(QLatin1Char(':'));
            if (colon <= 0)
                continue;
            const QString key = tag.left(colon).trimmed().toLower();
            const QString value = tag.mid(colon + 1).trimmed();
            if (key == QLatin1String("offset")) {
                bool ok = false;
                const qint64 parsed = value.toLongLong(&ok);
                if (ok)
                    offset = parsed;
            } else if (key == QLatin1String("ti")) {
                if (document.metadata().title.isEmpty())
                    document.metadata().title = value;
            } else if (key == QLatin1String("ar")) {
                if (document.metadata().artist.isEmpty())
                    document.metadata().artist = value;
            } else if (key == QLatin1String("al")) {
                if (document.metadata().album.isEmpty())
                    document.metadata().album = value;
            }
        }

        text = text.trimmed();
        if (text.isEmpty())
            continue;

        if (times.isEmpty()) {
            pending.append(Pending{-1, text});
        } else {
            for (qint64 time : times)
                pending.append(Pending{time, text});
            ++timedLines;
        }
    }

    if (timedLines == 0)
        return {std::nullopt, QStringLiteral("no timestamped lines")};

    for (Pending &item : pending) {
        if (item.start >= 0)
            item.start = qMax<qint64>(0, item.start + offset);
    }

    for (qsizetype i = 0; i < pending.size(); ++i) {
        const Pending &item = pending[i];
        Line &line = document.addLine();
        line.mainVocal.text = item.start >= 0 ? stripWordTags(item.text) : item.text;
        if (item.start < 0)
            continue;

        // A line lasts until the next timed line.
        qint64 end = -1;
        for (qsizetype j = i + 1; j < pending.size(); ++j) {
            if (pending[j].start >= 0) {
                end = pending[j].start;
                break;
            }
        }

        Timing timing;
        timing.start = Milliseconds(item.start);
        if (end >= item.start)
            timing.end = Milliseconds(end);
        line.mainVocal.timing = timing;

        end = buildWords(line, item.text, item.start, end, offset);
        line.mainVocal.timing->end = Milliseconds(end);
    }

    return {std::move(document), {}};
}

} // namespace lyrics
