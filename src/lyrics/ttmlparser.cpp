#include "ttmlparser.h"

#include <QHash>
#include <QRegularExpression>
#include <QStringList>
#include <QVector>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>

#include <chrono>
#include <utility>

namespace lyrics {

namespace {

using Milliseconds = std::chrono::milliseconds;

// TTML writes a time in three ways, all of them seconds underneath:
//   clock   "00:12.340"  "1:04.884"  "00:01:02.500"
//   offset  "12.340s"  "12340ms"  "2m"  "1h"
//   bare    "14.844"      - not spec-legal, but real files carry it
// Frames and ticks would need the rates the document never carries, so they
// are refused rather than guessed, and a time before the start of the track
// clamps to zero. Nothing is written to `out` on failure.
bool parseTime(QStringView value, qint64 *out)
{
    const QString text = value.trimmed().toString();
    if (text.isEmpty())
        return false;

    if (text.contains(QLatin1Char(':'))) {
        const QStringList parts = text.split(QLatin1Char(':'));
        if (parts.size() != 2 && parts.size() != 3)
            return false;
        bool secondsOk = false;
        const double seconds = parts.last().toDouble(&secondsOk);
        bool minutesOk = false;
        const double minutes = parts[parts.size() - 2].toDouble(&minutesOk);
        bool hoursOk = true;
        const double hours = parts.size() == 3 ? parts.first().toDouble(&hoursOk) : 0.0;
        if (!secondsOk || !minutesOk || !hoursOk)
            return false;
        const double millis = ((hours * 60.0 + minutes) * 60.0 + seconds) * 1000.0;
        *out = qMax<qint64>(0, qRound64(millis));
        return true;
    }

    static const QRegularExpression re(QStringLiteral("^(-?\\d+(?:\\.\\d+)?)\\s*(ms|s|m|h|f|t)?$"));
    const QRegularExpressionMatch match = re.match(text);
    if (!match.hasMatch())
        return false;
    bool ok = false;
    const double amount = match.captured(1).toDouble(&ok);
    if (!ok)
        return false;

    const QString unit = match.captured(2);
    if (unit == QLatin1String("f") || unit == QLatin1String("t"))
        return false;

    double millis = amount * 1000.0; // no unit at all, or "s": seconds
    if (unit == QLatin1String("ms"))
        millis = amount;
    else if (unit == QLatin1String("m"))
        millis = amount * 60000.0;
    else if (unit == QLatin1String("h"))
        millis = amount * 3600000.0;
    *out = qMax<qint64>(0, qRound64(millis));
    return true;
}

bool isAsciiLetter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isAsciiDigit(char c)
{
    return c >= '0' && c <= '9';
}

// The length of the entity reference starting at `&`, counting the `&` and the
// `;`, when it is one the reader resolves on its own: the five predefined
// names or a character reference. Anything else - a stray `&`, or a name
// nobody defines - is 0, and the caller rewrites it.
qsizetype resolvedEntityLength(const QByteArray &data, qsizetype at)
{
    qsizetype i = at + 1;
    if (i >= data.size())
        return 0;

    if (data.at(i) == '#') {
        ++i;
        const bool hex = i < data.size() && (data.at(i) == 'x' || data.at(i) == 'X');
        if (hex)
            ++i;
        const qsizetype digits = i;
        while (i < data.size() &&
               (hex ? (isAsciiDigit(data.at(i)) || (data.at(i) >= 'a' && data.at(i) <= 'f') ||
                       (data.at(i) >= 'A' && data.at(i) <= 'F'))
                    : isAsciiDigit(data.at(i))))
            ++i;
        if (i == digits)
            return 0;
        return i < data.size() && data.at(i) == ';' ? i - at + 1 : 0;
    }

    const qsizetype nameStart = i;
    while (i < data.size() && isAsciiLetter(data.at(i)))
        ++i;
    if (i == nameStart || i >= data.size() || data.at(i) != ';')
        return 0;
    const QByteArray name = data.mid(nameStart, i - nameStart);
    if (name != "amp" && name != "lt" && name != "gt" && name != "apos" && name != "quot")
        return 0;
    return i - at + 1;
}

// Rewrites every ampersand the reader would choke on as `&amp;`, byte for byte,
// so the document's declared encoding still decides the rest. This is what
// makes files with a bare `&` in the lyrics - Apple's own files carry them -
// parse at all; see `TtmlParser::parse`, which only ever repairs a document
// that already failed.
QByteArray escapeBareAmpersands(const QByteArray &data)
{
    QByteArray sanitized;
    sanitized.reserve(data.size() + 16);
    for (qsizetype i = 0; i < data.size(); ++i) {
        if (data.at(i) != '&') {
            sanitized.append(data.at(i));
            continue;
        }
        const qsizetype length = resolvedEntityLength(data, i);
        if (length > 0) {
            sanitized.append(data.mid(i, length));
            i += length - 1;
        } else {
            sanitized.append("&amp;");
        }
    }
    return sanitized;
}

// The value of an attribute by local name: the prefix (`ttm:agent`, `t:agent`,
// ...) belongs to the document, never to the meaning.
QStringView attribute(const QXmlStreamAttributes &attributes, QLatin1String localName)
{
    for (const QXmlStreamAttribute &entry : attributes) {
        if (entry.name() == localName)
            return entry.value();
    }
    return {};
}

// One run of text: a `<span>`, or bare text inside a paragraph. `start` and
// `end` stay -1 where the source gave no time for it, and `newWord` records
// whether whitespace preceded it in the source.
struct Run {
    QString text;
    qint64 start = -1;
    qint64 end = -1;
    bool newWord = false;
};

// One `<p>`: its times, who sings it, whether it backs another line, and the
// runs of text inside it.
struct Paragraph {
    QVector<Run> runs;
    qint64 begin = -1;
    qint64 end = -1;
    QString agent;
    bool background = false;
    bool wordTimed = false; ///< a span carries its own timing
};

// Adds one text run and remembers whether whitespace set it apart from the one
// before. Whitespace is the only thing that starts a new word: two runs glued
// together are syllables of the same word, which is what keeps Persian and
// Arabic fragments joined in the renderer.
void appendRun(const QString &raw, qint64 start, qint64 end, bool *pendingSpace,
               Paragraph *paragraph)
{
    static const QRegularExpression spaces(QStringLiteral("\\s+"));
    const QString collapsed = QString(raw).replace(spaces, QStringLiteral(" "));
    const bool separated = *pendingSpace || collapsed.startsWith(QLatin1Char(' '));
    const QString text = collapsed.trimmed();
    if (text.isEmpty()) {
        *pendingSpace = separated; // whitespace only: it separates, it is not text
        return;
    }

    Run run;
    run.text = text;
    run.start = start;
    run.end = end;
    run.newWord = paragraph->runs.isEmpty() || separated;
    paragraph->runs.append(std::move(run));
    *pendingSpace = false;
}

// Reads the content of the element the reader sits on, through its end tag. A
// nested `<span>` is where TTML puts a word's own times, so it opens runs of
// its own; anything else is skipped with its contents, `<br/>` at least keeping
// the words on either side apart. Returns false on malformed input.
bool collectRuns(QXmlStreamReader &xml, qint64 start, qint64 end, bool *pendingSpace,
                 Paragraph *paragraph)
{
    while (xml.readNext() != QXmlStreamReader::EndElement) {
        if (xml.atEnd() || xml.hasError())
            return false;

        if (xml.isCharacters()) {
            appendRun(xml.text().toString(), start, end, pendingSpace, paragraph);
            continue;
        }
        if (!xml.isStartElement())
            continue;

        const QStringView name = xml.name();
        if (name == QLatin1String("br")) {
            *pendingSpace = true;
            xml.skipCurrentElement();
            continue;
        }
        if (name != QLatin1String("span")) {
            xml.skipCurrentElement();
            continue;
        }

        const QXmlStreamAttributes attributes = xml.attributes();
        qint64 spanStart = -1;
        qint64 spanEnd = -1;
        const bool hasBegin = parseTime(attribute(attributes, QLatin1String("begin")), &spanStart);
        bool hasEnd = parseTime(attribute(attributes, QLatin1String("end")), &spanEnd);
        if (hasBegin && !hasEnd) {
            qint64 duration = -1;
            if (parseTime(attribute(attributes, QLatin1String("dur")), &duration)) {
                spanEnd = spanStart + duration;
                hasEnd = true;
            }
        }
        paragraph->wordTimed = paragraph->wordTimed || hasBegin || hasEnd;

        // A span inherits the paragraph's start but not its end: the end of a
        // span is finer than the line's, so leaving it open lets the next run
        // close it, where the paragraph's end would stretch the word to the
        // end of the line.
        if (!collectRuns(xml, hasBegin ? spanStart : start, hasEnd ? spanEnd : -1, pendingSpace,
                         paragraph))
            return false;
    }
    return !xml.hasError();
}

// Reads one `<p>`: its times, who sings it, whether it backs another line, and
// its runs. The reader ends past the paragraph's end tag.
bool readParagraph(QXmlStreamReader &xml, Paragraph *paragraph)
{
    const QXmlStreamAttributes attributes = xml.attributes();
    parseTime(attribute(attributes, QLatin1String("begin")), &paragraph->begin);
    if (!parseTime(attribute(attributes, QLatin1String("end")), &paragraph->end)) {
        // `dur` is the other way TTML says how long something lasts.
        qint64 duration = -1;
        if (parseTime(attribute(attributes, QLatin1String("dur")), &duration) &&
            paragraph->begin >= 0)
            paragraph->end = paragraph->begin + duration;
    }
    paragraph->agent = attribute(attributes, QLatin1String("agent")).toString();
    // `x-bg` is Apple's background-vocal marker; any other role is ordinary.
    paragraph->background = attribute(attributes, QLatin1String("role")) == QLatin1String("x-bg");

    bool pendingSpace = false;
    return collectRuns(xml, paragraph->begin, paragraph->end, &pendingSpace, paragraph);
}

// Groups the runs into words and closes the timing the source left open. A
// word starts with its first run and ends with its last; anything still
// missing borrows the neighbour that fills it - the next run, the next word,
// the paragraph's end - in that order, the way the LRC parser does it, so both
// formats read alike.
void buildWords(Vocal *vocal, const Paragraph &paragraph)
{
    struct WordBuild {
        QString text;
        qint64 start = -1;
        qint64 end = -1;
        QVector<Run> parts;
    };

    QVector<WordBuild> words;
    for (const Run &run : paragraph.runs) {
        if (words.isEmpty() || run.newWord)
            words.append(WordBuild{});
        WordBuild &word = words.last();
        word.text += run.text;
        if (word.start < 0 && run.start >= 0)
            word.start = run.start;
        word.parts.append(run);
    }

    const qint64 fallbackEnd = paragraph.end;
    qint64 cursor = paragraph.begin;
    for (qsizetype i = 0; i < words.size(); ++i) {
        WordBuild &word = words[i];
        if (word.start < 0)
            word.start = cursor;
        if (cursor >= 0 && word.start < cursor)
            word.start = cursor;

        qint64 wordEnd = -1;
        for (const Run &part : word.parts)
            wordEnd = qMax(wordEnd, part.end);
        if (wordEnd < 0)
            wordEnd = (i + 1 < words.size() && words[i + 1].start >= 0) ? words[i + 1].start
                                                                        : fallbackEnd;
        if (wordEnd >= word.start)
            word.end = wordEnd;

        for (qsizetype k = 0; k < word.parts.size(); ++k) {
            Run &part = word.parts[k];
            if (part.start < 0)
                part.start = word.start;
            if (part.end < 0)
                part.end = (k + 1 < word.parts.size() && word.parts[k + 1].start >= 0)
                               ? word.parts[k + 1].start
                               : word.end;
            if (part.start >= 0 && part.end < part.start)
                part.end = part.start;
        }

        if (word.end >= 0)
            cursor = word.end;
    }

    vocal->words.reserve(vocal->words.size() + words.size());
    for (const WordBuild &build : std::as_const(words)) {
        Word word;
        word.text = build.text;
        if (build.start >= 0) {
            Timing timing;
            timing.start = Milliseconds(build.start);
            if (build.end >= build.start)
                timing.end = Milliseconds(build.end);
            word.timing = timing;
        }
        // Several runs only become syllables when the source split them, so
        // nothing invents a precision that was not there.
        if (build.parts.size() > 1) {
            word.syllables.reserve(build.parts.size());
            for (const Run &part : build.parts) {
                Syllable syllable;
                syllable.text = part.text;
                if (part.start >= 0) {
                    Timing timing;
                    timing.start = Milliseconds(part.start);
                    if (part.end >= part.start)
                        timing.end = Milliseconds(part.end);
                    syllable.timing = timing;
                }
                word.syllables.push_back(std::move(syllable));
            }
        }
        vocal->words.push_back(std::move(word));
    }
}

// Builds a vocal from a paragraph. Words only appear when the source timed its
// spans itself: a paragraph of plain text stays a line with no words, the way
// an LRC line without `<mm:ss.xx>` tags does, so it shows as a plain line
// instead of being swept as karaoke.
Vocal buildVocal(const Paragraph &paragraph)
{
    Vocal vocal;
    for (const Run &run : paragraph.runs) {
        if (!vocal.text.isEmpty() && run.newWord)
            vocal.text += QLatin1Char(' ');
        vocal.text += run.text;
    }

    qint64 start = paragraph.begin;
    if (paragraph.wordTimed && !paragraph.runs.isEmpty()) {
        buildWords(&vocal, paragraph);
        if (start < 0 && !vocal.words.empty() && vocal.words.front().timing)
            start = static_cast<qint64>(vocal.words.front().timing->start.count());
    }

    if (start >= 0) {
        Timing timing;
        timing.start = Milliseconds(start);
        if (paragraph.end >= start)
            timing.end = Milliseconds(paragraph.end);
        vocal.timing = timing;
    }
    return vocal;
}

// One pass over the document: the root, the agents the head declares, and every
// paragraph in reading order.
ParseResult parseDocument(const QByteArray &data)
{
    if (data.isEmpty())
        return {std::nullopt, QStringLiteral("empty input")};

    QXmlStreamReader xml(data);
    if (!xml.readNextStartElement() || xml.name() != QLatin1String("tt"))
        return {std::nullopt, QStringLiteral("not a TTML document")};

    Lyrics document;

    // `<ttm:agent xml:id="v1" type="person"/>` in the head says what an id
    // means; the id is also the name, the same "v1", "v1000" LRC reads from a
    // "v1:" prefix. Agents are created on first use, so they come out in order
    // of appearance - which is what puts the singers of a duet on opposite
    // sides.
    struct AgentDecl {
        AgentType type = AgentType::Other;
        QString name;
    };
    QHash<QString, AgentDecl> declarations;
    QHash<QString, Id> agents;
    const auto agentFor = [&](const QString &id) -> Id {
        if (id.isEmpty())
            return InvalidId;
        const auto existing = agents.constFind(id);
        if (existing != agents.constEnd())
            return existing.value();

        AgentDecl decl;
        const auto declared = declarations.constFind(id);
        if (declared != declarations.constEnd()) {
            decl = declared.value();
        } else {
            // Undeclared ids still follow Apple's numbering: from 1000 up they
            // name a group, below that a person.
            static const QRegularExpression trailingDigits(QStringLiteral("\\d+$"));
            const QRegularExpressionMatch match = trailingDigits.match(id);
            bool ok = false;
            const int number = match.captured(0).toInt(&ok);
            if (ok)
                decl.type = number >= 1000 ? AgentType::Group : AgentType::Person;
        }

        const QString name = decl.name.isEmpty() ? id : decl.name;
        const Id agentId = document.addAgent(decl.type, name);
        agents.insert(id, agentId);
        return agentId;
    };

    bool inHead = false;
    qsizetype paragraphCount = 0;

    while (!xml.atEnd()) {
        const auto token = xml.readNext();
        if (token == QXmlStreamReader::Invalid)
            break;

        if (token == QXmlStreamReader::EndElement) {
            if (xml.name() == QLatin1String("head"))
                inHead = false;
            continue;
        }
        if (token != QXmlStreamReader::StartElement)
            continue;

        const QStringView name = xml.name();
        if (name == QLatin1String("head")) {
            inHead = true;
        } else if (inHead && name == QLatin1String("agent")) {
            const QXmlStreamAttributes attributes = xml.attributes();
            const QString id = attribute(attributes, QLatin1String("id")).toString();
            if (!id.isEmpty()) {
                AgentDecl decl;
                const QString type = attribute(attributes, QLatin1String("type")).toString();
                if (type == QLatin1String("person"))
                    decl.type = AgentType::Person;
                else if (type == QLatin1String("group"))
                    decl.type = AgentType::Group;
                decl.name = attribute(attributes, QLatin1String("name")).toString();
                declarations.insert(id, decl);
            }
            xml.skipCurrentElement();
        } else if (name == QLatin1String("p")) {
            Paragraph paragraph;
            if (!readParagraph(xml, &paragraph))
                break;
            // A paragraph with no text is nothing to sing, so it is left out
            // entirely instead of becoming an empty row.
            if (paragraph.runs.isEmpty())
                continue;

            Line &line = document.addLine();
            const Id agentId = agentFor(paragraph.agent);
            Vocal vocal = buildVocal(paragraph);
            vocal.agentId = agentId;

            if (paragraph.background) {
                // A background has no line of its own here: its row is the
                // whole element. The paragraph's times still belong to it, so
                // the element keeps its window while the text lives below.
                line.mainVocal.timing = vocal.timing;
                line.mainVocal.agentId = agentId;
                line.backgrounds.push_back(std::move(vocal));
            } else {
                line.mainVocal = std::move(vocal);
            }
            ++paragraphCount;
        }
    }

    if (xml.hasError())
        return {std::nullopt, xml.errorString()};
    if (paragraphCount == 0)
        return {std::nullopt, QStringLiteral("no lyric lines")};

    return {std::move(document), {}};
}

} // namespace

QString TtmlParser::formatName() const
{
    return QStringLiteral("ttml");
}

ParseResult TtmlParser::parse(const QByteArray &data) const
{
    ParseResult result = parseDocument(data);
    if (result.succeeded())
        return result;

    // A bare `&` is common enough - Apple's own files carry one - that losing
    // the file over it would be worse than fixing it. Only a document that
    // already failed gets this far, so a well-formed one is parsed exactly as
    // it is written.
    const QByteArray repaired = escapeBareAmpersands(data);
    if (repaired == data)
        return result;

    ParseResult repairedResult = parseDocument(repaired);
    if (repairedResult.succeeded())
        return repairedResult;
    return result;
}

} // namespace lyrics
