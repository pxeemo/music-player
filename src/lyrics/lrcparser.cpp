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

// Some files prefix a line with the vocalist, e.g. "v1:" or "v2:".
QString stripVocalist(QString text)
{
    static const QRegularExpression re(QStringLiteral("^\\s*v\\d+\\s*:\\s*"),
                                       QRegularExpression::CaseInsensitiveOption);
    text.remove(re);
    return text;
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

struct Fragment {
    QString text;
    qint64 start = -1;
    qint64 end = -1;
    bool newWord = false;
};

// Parses enhanced-LRC word fragments. `<mm:ss.xx>` tags mark a boundary: they
// close the previous fragment and open the next. Fragments with no whitespace
// between them are syllables of one word (the tags are removed, so scripts that
// join - Arabic, Persian - stay connected); whitespace starts a new word.
//
// Returns the line's end in ms, or -1 when the line has no word timing at all.
qint64 parseWords(Line &line, const QString &text, qint64 lineStart, qint64 fallbackEnd,
                  qint64 offset)
{
    static const QRegularExpression tokenRe(
        QStringLiteral("(\\s*)(<\\s*\\d{1,3}:\\d{1,2}(?:[.:]\\d{1,3})?\\s*>|[^\\s<]+)"));
    static const QRegularExpression tagRe(QStringLiteral("^<\\s*(.*?)\\s*>$"));

    QVector<Fragment> fragments;
    qint64 nextStart = -1;
    qint64 lastTag = -1;
    bool boundary = false;

    QRegularExpressionMatchIterator it = tokenRe.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const bool hadSpace = !match.captured(1).isEmpty();
        const QString token = match.captured(2);

        if (token.startsWith(QLatin1Char('<'))) {
            const QRegularExpressionMatch tagMatch = tagRe.match(token);
            qint64 parsed = 0;
            if (tagMatch.hasMatch() && parseTimestamp(tagMatch.captured(1), &parsed)) {
                const qint64 time = qMax<qint64>(0, parsed + offset);
                if (!fragments.isEmpty() && fragments.back().end < 0)
                    fragments.back().end = time;
                nextStart = time;
                lastTag = time;
                boundary = boundary || hadSpace;
            }
            continue;
        }

        Fragment fragment;
        fragment.text = token;
        fragment.start = nextStart;
        fragment.newWord = fragments.isEmpty() || hadSpace || boundary;
        fragments.append(fragment);
        nextStart = -1;
        boundary = false;
    }

    if (fragments.isEmpty() || lastTag < 0)
        return -1;

    // A tag directly after the last fragment gives the true end; otherwise the
    // line runs until the next one (the caller's fallback).
    const qint64 knownEnd = fragments.back().end;
    const qint64 lineEnd = knownEnd >= lineStart ? knownEnd : fallbackEnd;

    struct WordBuild {
        QString text;
        qint64 start = -1;
        qint64 end = -1;
        QVector<Fragment> parts;
    };

    QVector<WordBuild> words;
    for (const Fragment &fragment : fragments) {
        if (words.isEmpty() || fragment.newWord)
            words.append(WordBuild{});
        WordBuild &word = words.last();
        word.text += fragment.text;
        if (word.start < 0 && fragment.start >= 0)
            word.start = fragment.start;
        word.parts.append(fragment);
    }

    qint64 cursor = lineStart;
    for (qsizetype i = 0; i < words.size(); ++i) {
        WordBuild &word = words[i];
        if (word.start < 0)
            word.start = cursor;
        if (word.start < cursor)
            word.start = cursor;

        qint64 wordEnd = -1;
        for (qsizetype k = 0; k < word.parts.size(); ++k) {
            Fragment &part = word.parts[k];
            if (part.end < 0 && k + 1 < word.parts.size() && word.parts[k + 1].start >= 0)
                part.end = word.parts[k + 1].start;
            wordEnd = qMax(wordEnd, part.end);
        }
        if (wordEnd < 0)
            wordEnd = (i + 1 < words.size() && words[i + 1].start >= 0) ? words[i + 1].start
                                                                        : lineEnd;
        if (wordEnd < word.start)
            wordEnd = word.start;
        word.end = wordEnd;

        for (qsizetype k = 0; k < word.parts.size(); ++k) {
            Fragment &part = word.parts[k];
            if (part.start < 0)
                part.start = word.start;
            if (part.end < 0)
                part.end = (k + 1 < word.parts.size() && word.parts[k + 1].start >= 0)
                               ? word.parts[k + 1].start
                               : word.end;
            if (part.end < part.start)
                part.end = part.start;
        }
        cursor = word.end;
    }

    line.mainVocal.words.reserve(line.mainVocal.words.size() + words.size());
    for (const WordBuild &build : words) {
        Word word;
        word.text = build.text;
        word.timing = Timing{Milliseconds(build.start), Milliseconds(build.end)};
        if (build.parts.size() > 1) {
            word.syllables.reserve(build.parts.size());
            for (const Fragment &part : build.parts) {
                Syllable syllable;
                syllable.text = part.text;
                syllable.timing = Timing{Milliseconds(part.start), Milliseconds(part.end)};
                word.syllables.push_back(std::move(syllable));
            }
        }
        line.mainVocal.words.push_back(std::move(word));
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

        text = stripVocalist(text.trimmed());
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

        // A line lasts until the next timed line, unless its own word tags give
        // an earlier end.
        qint64 nextStart = -1;
        for (qsizetype j = i + 1; j < pending.size(); ++j) {
            if (pending[j].start >= 0) {
                nextStart = pending[j].start;
                break;
            }
        }

        const qint64 end = parseWords(line, item.text, item.start, nextStart, offset);

        Timing timing;
        timing.start = Milliseconds(item.start);
        if (end >= item.start)
            timing.end = Milliseconds(end);
        line.mainVocal.timing = timing;
    }

    return {std::move(document), {}};
}

} // namespace lyrics
