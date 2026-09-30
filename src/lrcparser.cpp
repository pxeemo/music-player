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
// count, so ".5" and ".50" and ".500" all mean 500 ms.
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

// Splits enhanced-LRC text into words with starts taken from `<mm:ss.xx>` tags.
// Words that carry no tag inherit the previous word's end. Ends are filled in
// afterwards (next word, or the line end).
void buildWords(Line &line, const QString &text, qint64 lineStart, qint64 lineEnd, qint64 offset)
{
	static const QRegularExpression token(
		QStringLiteral("<\\s*\\d{1,3}:\\d{1,2}(?:[.:]\\d{1,3})?\\s*>|[^\\s<]+"));
	static const QRegularExpression tag(QStringLiteral("^<\\s*(.*?)\\s*>$"));

	QVector<QString> texts;
	QVector<qint64> starts;
	qint64 current = -1;
	bool sawTag = false;

	QRegularExpressionMatchIterator it = token.globalMatch(text);
	while (it.hasNext()) {
		const QRegularExpressionMatch match = it.next();
		const QString tokenText = match.captured(0);
		if (tokenText.startsWith(QLatin1Char('<'))) {
			const QRegularExpressionMatch tagMatch = tag.match(tokenText);
			qint64 parsed = 0;
			if (tagMatch.hasMatch() && parseTimestamp(tagMatch.captured(1), &parsed)) {
				current = qMax<qint64>(0, parsed + offset);
				sawTag = true;
			}
			continue;
		}
		texts.append(tokenText);
		starts.append(current);
		current = -1;
	}

	if (!sawTag || texts.isEmpty())
		return;

	// Resolve starts: missing ones continue from the previous word.
	qint64 cursor = lineStart;
	for (qsizetype i = 0; i < starts.size(); ++i) {
		if (starts[i] < 0)
			starts[i] = cursor;
		if (starts[i] < cursor)
			starts[i] = cursor;
		cursor = starts[i];
	}

	line.mainVocal.words.reserve(std::size_t(texts.size()));
	for (qsizetype i = 0; i < texts.size(); ++i) {
		const qint64 end = (i + 1 < starts.size()) ? starts[i + 1] : lineEnd;
		Word word;
		word.text = texts[i];
		Timing timing;
		timing.start = Milliseconds(starts[i]);
		if (end >= starts[i])
			timing.end = Milliseconds(end);
		word.timing = timing;
		line.mainVocal.words.push_back(std::move(word));
	}
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

		buildWords(line, item.text, item.start, end, offset);
	}

	return {std::move(document), {}};
}

} // namespace lyrics
