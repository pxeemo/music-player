#include "lyricsdocument.h"

#include <QStringList>
#include <QVector>

#include <utility>

namespace {

qint64 toMs(const lyrics::Timestamp &timestamp)
{
	return static_cast<qint64>(timestamp.count());
}

// Best-effort word timing for one vocal. Word timing wins; a word without its
// own timing falls back to the span of its syllables. This never invents a
// precision the source did not provide - if nothing is timed, the result is
// "not karaoke".
LyricRow::Timed buildTimed(const lyrics::Vocal &vocal, qint64 lineStartMs, qint64 lineEndMs)
{
	const qsizetype count = static_cast<qsizetype>(vocal.words.size());
	if (count == 0)
		return {};

	QVector<qint64> start(count, -1);
	QVector<qint64> end(count, -1);
	bool anyTimed = false;
	for (qsizetype i = 0; i < count; ++i) {
		const lyrics::Word &word = vocal.words[static_cast<std::size_t>(i)];
		if (word.timing) {
			start[i] = toMs(word.timing->start);
			if (word.timing->end)
				end[i] = toMs(*word.timing->end);
			anyTimed = true;
		} else if (!word.syllables.empty()) {
			for (const lyrics::Syllable &syllable : word.syllables) {
				if (!syllable.timing)
					continue;
				const qint64 s = toMs(syllable.timing->start);
				const qint64 e = syllable.timing->end ? toMs(*syllable.timing->end) : s;
				if (start[i] < 0 || s < start[i])
					start[i] = s;
				if (end[i] < 0 || e > end[i])
					end[i] = e;
				anyTimed = true;
			}
		}
	}
	if (!anyTimed)
		return {};

	qint64 lineStart = lineStartMs;
	qint64 lineEnd = lineEndMs;
	for (qsizetype i = 0; i < count && lineStart < 0; ++i)
		lineStart = start[i];
	for (qsizetype i = count - 1; i >= 0 && lineEnd < 0; --i)
		lineEnd = end[i];
	if (lineStart < 0)
		lineStart = 0;
	if (lineEnd < 0)
		lineEnd = lineStart;

	// Fill the gaps so the renderer always sees a monotonic, fully timed list.
	qint64 cursor = lineStart;
	for (qsizetype i = 0; i < count; ++i) {
		if (start[i] < 0)
			start[i] = cursor;
		if (start[i] < cursor)
			start[i] = cursor;
		cursor = start[i];
	}
	for (qsizetype i = 0; i < count; ++i) {
		if (end[i] < 0)
			end[i] = (i + 1 < count) ? start[i + 1] : lineEnd;
		if (end[i] < start[i])
			end[i] = start[i];
	}

	LyricRow::Timed timed;
	timed.karaoke = true;
	timed.lineStart = lineStart / 1000.0;
	timed.lineEnd = lineEnd / 1000.0;
	timed.words.reserve(count);
	for (qsizetype i = 0; i < count; ++i) {
		timed.words.append(LyricRow::WordSpec{
			vocal.words[static_cast<std::size_t>(i)].text, start[i] / 1000.0, end[i] / 1000.0});
	}
	return timed;
}

QString formatTimestamp(const lyrics::Timestamp &timestamp)
{
	const qint64 total = toMs(timestamp);
	const qint64 minutes = total / 60000;
	const qint64 seconds = (total % 60000) / 1000;
	const qint64 millis = total % 1000;
	return QStringLiteral("%1:%2.%3")
		.arg(minutes)
		.arg(seconds, 2, 10, QLatin1Char('0'))
		.arg(millis, 3, 10, QLatin1Char('0'));
}

QString formatTiming(const lyrics::OptionalTiming &timing)
{
	if (!timing)
		return QStringLiteral("(untimed)");
	QString result = formatTimestamp(timing->start);
	if (timing->end)
		result += QStringLiteral("..") + formatTimestamp(*timing->end);
	return result;
}

QString agentTypeName(lyrics::AgentType type)
{
	switch (type) {
	case lyrics::AgentType::Person:
		return QStringLiteral("person");
	case lyrics::AgentType::Group:
		return QStringLiteral("group");
	case lyrics::AgentType::Other:
		return QStringLiteral("other");
	}
	return QStringLiteral("other");
}

} // namespace

// ---------------------------------------------------------------------------
// TimedWord
// ---------------------------------------------------------------------------

TimedWord::TimedWord(QString text, qreal start, qreal end, QObject *parent)
	: QObject(parent), m_text(std::move(text)), m_start(start), m_end(end)
{
}

// ---------------------------------------------------------------------------
// LyricRow
// ---------------------------------------------------------------------------

LyricRow::LyricRow(Kind kind, QString text, QString agentName, qint64 startMs, qint64 groupStartMs,
				   int groupIndex, bool groupStart, Timed timed, QObject *parent)
	: QObject(parent), m_kind(kind), m_text(std::move(text)), m_agentName(std::move(agentName)),
	  m_startMs(startMs), m_groupStartMs(groupStartMs), m_groupIndex(groupIndex),
	  m_groupStart(groupStart), m_karaoke(timed.karaoke), m_lineStart(timed.lineStart),
	  m_lineEnd(timed.lineEnd)
{
	m_words.reserve(timed.words.size());
	for (const WordSpec &spec : timed.words)
		m_words.append(new TimedWord(spec.text, spec.start, spec.end, this));
}

// ---------------------------------------------------------------------------
// LyricsDocument
// ---------------------------------------------------------------------------

LyricsDocument::LyricsDocument(QObject *parent) : QObject(parent) {}

LyricsDocument::~LyricsDocument() = default;

void LyricsDocument::setLyrics(lyrics::Lyrics &&document)
{
	m_lyrics = std::move(document);
	m_hasLyrics = true;
	rebuild();
}

LyricRow *LyricsDocument::addRow(LyricRow::Kind kind, const QString &text, const QString &agentName,
								 qint64 startMs, qint64 groupStartMs, int groupIndex, bool groupStart,
								 LyricRow::Timed timed)
{
	auto *row = new LyricRow(kind, text, agentName, startMs, groupStartMs, groupIndex, groupStart,
							 std::move(timed), this);
	if (groupStartMs >= 0)
		m_timed = true;
	m_rows.append(row);
	return row;
}

QString LyricsDocument::agentName(lyrics::Id id) const
{
	if (const lyrics::Agent *agent = m_lyrics.agent(id))
		return agent->name;
	return {};
}

void LyricsDocument::rebuild()
{
	qDeleteAll(m_rows);
	m_rows.clear();
	m_timed = false;

	int groupIndex = 0;
	for (const std::unique_ptr<lyrics::Element> &element : m_lyrics.elements()) {
		const int group = groupIndex++;
		switch (element->kind()) {
		case lyrics::ElementKind::Section: {
			const auto *section = static_cast<const lyrics::Section *>(element.get());
			const qint64 start = section->timing ? toMs(section->timing->start) : -1;
			addRow(LyricRow::Section, section->name, {}, start, start, group, true);
			break;
		}
		case lyrics::ElementKind::Instrumental: {
			const auto *instrumental = static_cast<const lyrics::Instrumental *>(element.get());
			const QString text = instrumental->description.isEmpty()
									 ? QStringLiteral("\u266A instrumental")
									 : instrumental->description;
			const qint64 start = instrumental->timing ? toMs(instrumental->timing->start) : -1;
			addRow(LyricRow::Instrumental, text, {}, start, start, group, true);
			break;
		}
		case lyrics::ElementKind::Line: {
			const auto *line = static_cast<const lyrics::Line *>(element.get());
			const qint64 lineStart =
				line->mainVocal.timing ? toMs(line->mainVocal.timing->start) : -1;
			const qint64 lineEnd =
				(line->mainVocal.timing && line->mainVocal.timing->end)
					? toMs(*line->mainVocal.timing->end)
					: -1;

			addRow(LyricRow::Main, line->mainVocal.text, agentName(line->mainVocal.agentId),
				   lineStart, lineStart, group, true,
				   buildTimed(line->mainVocal, lineStart, lineEnd));

			// Translations inherit the line's timing, so they highlight with it
			// and a click seeks to the line's start.
			for (const lyrics::Translation &translation : line->translations)
				addRow(LyricRow::Translation, translation.text, {}, lineStart, lineStart, group,
					   false);

			// Background vocals carry their own timing when the source gives it.
			for (const lyrics::Vocal &background : line->backgrounds) {
				const qint64 backgroundStart =
					background.timing ? toMs(background.timing->start) : lineStart;
				const qint64 groupStart = lineStart >= 0 ? lineStart : backgroundStart;
				addRow(LyricRow::Background, background.text, agentName(background.agentId),
					   backgroundStart, groupStart, group, false);
			}
			break;
		}
		}
	}

	emit rowsChanged();
}

LyricRow *LyricsDocument::rowAt(int index) const
{
	if (index < 0 || index >= m_rows.size())
		return nullptr;
	return m_rows.at(index);
}

int LyricsDocument::activeGroupIndex(qint64 positionMs) const
{
	int active = -1;
	qint64 best = -1;
	for (const LyricRow *row : m_rows) {
		if (!row->groupStart())
			continue;
		const qint64 start = row->groupStartMs();
		if (start < 0 || start > positionMs)
			continue;
		if (start >= best) {
			best = start;
			active = row->groupIndex();
		}
	}
	return active;
}

int LyricsDocument::firstRowOfGroup(int groupIndex) const
{
	for (int i = 0; i < m_rows.size(); ++i) {
		if (m_rows.at(i)->groupIndex() == groupIndex && m_rows.at(i)->groupStart())
			return i;
	}
	return -1;
}

QString LyricsDocument::debugText() const
{
	if (!m_hasLyrics)
		return QStringLiteral("(no lyrics document)");

	QStringList out;
	const lyrics::Metadata &meta = m_lyrics.metadata();
	out << QStringLiteral("metadata:")
		<< QStringLiteral("  title: ") + meta.title
		<< QStringLiteral("  artist: ") + meta.artist
		<< QStringLiteral("  album: ") + meta.album
		<< QStringLiteral("  language: ") + meta.language;
	out << QStringLiteral("agents: ") + QString::number(m_lyrics.agents().size());
	for (const lyrics::Agent &agent : m_lyrics.agents()) {
		out << QStringLiteral("  #%1 %2 \"%3\"")
				   .arg(agent.id)
				   .arg(agentTypeName(agent.type))
				   .arg(agent.name);
	}
	out << QStringLiteral("elements: ") + QString::number(m_lyrics.elementCount());

	for (const std::unique_ptr<lyrics::Element> &element : m_lyrics.elements()) {
		switch (element->kind()) {
		case lyrics::ElementKind::Section: {
			const auto *section = static_cast<const lyrics::Section *>(element.get());
			out << QStringLiteral("  [section] \"%1\" %2")
					   .arg(section->name, formatTiming(section->timing));
			break;
		}
		case lyrics::ElementKind::Instrumental: {
			const auto *instrumental = static_cast<const lyrics::Instrumental *>(element.get());
			out << QStringLiteral("  [instrumental] \"%1\" %2")
					   .arg(instrumental->description, formatTiming(instrumental->timing));
			break;
		}
		case lyrics::ElementKind::Line: {
			const auto *line = static_cast<const lyrics::Line *>(element.get());
			const lyrics::Agent *who = m_lyrics.agent(line->mainVocal.agentId);
			QString header = QStringLiteral("  [line #%1] \"%2\" %3")
								 .arg(line->id())
								 .arg(line->mainVocal.text, formatTiming(line->mainVocal.timing));
			if (who)
				header += QStringLiteral(" agent=") + who->name;
			out << header;
			for (const lyrics::Word &word : line->mainVocal.words) {
				out << QStringLiteral("    word \"%1\" %2").arg(word.text, formatTiming(word.timing));
				for (const lyrics::Syllable &syllable : word.syllables)
					out << QStringLiteral("      syllable \"%1\" %2")
							   .arg(syllable.text, formatTiming(syllable.timing));
			}
			for (const lyrics::Translation &translation : line->translations)
				out << QStringLiteral("    translation [%1] \"%2\"")
						   .arg(translation.language, translation.text);
			for (const lyrics::Vocal &background : line->backgrounds) {
				const lyrics::Agent *backgroundAgent = m_lyrics.agent(background.agentId);
				QString row = QStringLiteral("    background \"%1\" %2")
								  .arg(background.text, formatTiming(background.timing));
				if (backgroundAgent)
					row += QStringLiteral(" agent=") + backgroundAgent->name;
				out << row;
			}
			break;
		}
		}
	}
	return out.join(QLatin1Char('\n'));
}
