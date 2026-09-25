#include "lyrics.h"

#include <initializer_list>

QString LyricLine::text() const
{
<<<<<<< HEAD
	QString result;
	for (const Word *word : m_words) {
		if (!result.isEmpty())
			result += QLatin1Char(' ');
		result += word->text();
	}
	return result;
}


qreal LyricLine::start() const
{
	return m_words.isEmpty() ? 0.0 : m_words.first()->start();
=======
    QString result;
    for (const Word *word : m_words) {
        if (!result.isEmpty())
            result += QLatin1Char(' ');
        result += word->text();
    }
    return result;
}

qreal LyricLine::start() const
{
    return m_words.isEmpty() ? 0.0 : m_words.first()->start();
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
}

qreal LyricLine::end() const
{
<<<<<<< HEAD
	return m_words.isEmpty() ? 0.0 : m_words.last()->end();
=======
    return m_words.isEmpty() ? 0.0 : m_words.last()->end();
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
}

Word *LyricLine::addWord(const QString &text, qreal start, qreal end)
{
<<<<<<< HEAD
	auto *word = new Word(this);
	word->setText(text);
	word->setStart(start);
	word->setEnd(end);
	m_words.append(word);
	emit wordsChanged();
	return word;
=======
    auto *word = new Word(this);
    word->setText(text);
    word->setStart(start);
    word->setEnd(end);
    m_words.append(word);
    emit wordsChanged();
    return word;
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
}

void LyricLine::clear()
{
<<<<<<< HEAD
	qDeleteAll(m_words);
	m_words.clear();
	emit wordsChanged();
=======
    qDeleteAll(m_words);
    m_words.clear();
    emit wordsChanged();
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
}

qreal LyricsModel::duration() const
{
<<<<<<< HEAD
	return m_lines.isEmpty() ? 0.0 : m_lines.last()->end();
=======
    return m_lines.isEmpty() ? 0.0 : m_lines.last()->end();
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
}

LyricLine *LyricsModel::lineAt(int index) const
{
<<<<<<< HEAD
	if (index < 0 || index >= m_lines.size())
		return nullptr;
	return m_lines.at(index);
=======
    if (index < 0 || index >= m_lines.size())
        return nullptr;
    return m_lines.at(index);
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
}

void LyricsModel::notifyLinesChanged()
{
<<<<<<< HEAD
	if (!m_bulkLoading)
		emit linesChanged();
=======
    if (!m_bulkLoading)
        emit linesChanged();
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
}

LyricLine *LyricsModel::addLine()
{
<<<<<<< HEAD
	auto *line = new LyricLine(this);
	// A line's start/end are derived from its words, and so is duration() above.
	// Without this forward, QML bindings that read duration would never learn
	// that words arrived after the line itself was appended.
	connect(line, &LyricLine::wordsChanged, this, &LyricsModel::notifyLinesChanged);
	m_lines.append(line);
	notifyLinesChanged();
	return line;
=======
    auto *line = new LyricLine(this);
    // A line's start/end are derived from its words, and so is duration() above.
    // Without this forward, QML bindings that read duration would never learn
    // that words arrived after the line itself was appended.
    connect(line, &LyricLine::wordsChanged, this, &LyricsModel::notifyLinesChanged);
    m_lines.append(line);
    notifyLinesChanged();
    return line;
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
}

void LyricsModel::clear()
{
<<<<<<< HEAD
	qDeleteAll(m_lines);
	m_lines.clear();
	notifyLinesChanged();
=======
    qDeleteAll(m_lines);
    m_lines.clear();
    notifyLinesChanged();
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
}

int LyricsModel::activeLine(qreal position) const
{
<<<<<<< HEAD
	int active = -1;
	for (int i = 0; i < m_lines.size(); ++i) {
		if (m_lines.at(i)->start() <= position)
			active = i;
		else
			break;
	}
	return active;
=======
    int active = -1;
    for (int i = 0; i < m_lines.size(); ++i) {
        if (m_lines.at(i)->start() <= position)
            active = i;
        else
            break;
    }
    return active;
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
}

// One entry of the demo data below: a word with its time range in seconds.
namespace {
struct DemoWord {
<<<<<<< HEAD
	const char *text;
	qreal start;
	qreal end;
=======
    const char *text;
    qreal start;
    qreal end;
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
};
} // namespace

// Hard-coded demo timings, long enough (~63 s, 16 lines) to overflow the window
// and exercise scrolling. The shape of this data is the contract with the
// renderer; a parser would produce the same thing.
void LyricsModel::loadDemoLyrics()
{
<<<<<<< HEAD
	// Every addWord() would otherwise fan out to linesChanged(), and the QML
	// Repeater would rebuild every delegate once per word. Load silently and
	// announce the finished model once at the end.
	m_bulkLoading = true;
	clear();

	const auto line = [this](std::initializer_list<DemoWord> words) {
		LyricLine *l = addLine();
		for (const DemoWord &word : words)
			l->addWord(QString::fromUtf8(word.text), word.start, word.end);
	};

	line({{"I", 0.60, 0.90}, {"remember", 1.00, 2.05}, {"when", 2.20, 3.60}});
	line({{"we", 4.00, 4.30}, {"used", 4.40, 5.10}, {"to", 5.20, 5.55}, {"stay", 5.70, 7.00}});
	line({{"under", 7.45, 8.15}, {"the", 8.25, 8.65}, {"summer", 8.80, 10.40}});
	line({{"sky", 10.85, 11.45}, {"together", 11.60, 13.60}});
	line({{"تست", 13.60, 14.00}, {"فارسی", 14.00, 15.50}});

	line({{"counting", 15.90, 16.55}, {"every", 16.65, 17.20},
		  {"little", 17.30, 17.85}, {"thing", 17.95, 19.30}});
	line({{"we", 19.70, 20.05}, {"promised", 20.15, 21.00}, {"not", 21.10, 21.45},
		  {"to", 21.55, 21.85}, {"forget", 21.95, 23.30}});
	line({{"the", 23.70, 24.00}, {"streetlights", 24.10, 25.25}, {"hummed", 25.35, 26.30}});
	line({{"our", 26.70, 27.10}, {"names", 27.20, 28.30}, {"in", 28.40, 28.75},
		  {"the", 28.85, 29.15}, {"dark", 29.25, 30.60}});
	line({{"and", 31.00, 31.40}, {"no", 31.50, 31.90}, {"one", 32.00, 32.55},
		  {"ever", 32.65, 33.40}, {"asked", 33.50, 34.80}});
	line({{"why", 35.20, 35.70}, {"we", 35.80, 36.15}, {"stayed", 36.25, 37.10},
		  {"so", 37.20, 37.60}, {"late", 37.70, 39.00}});

	// A second right-to-left line, further down the list, so scrolling and
	// bidi shaping are tested together.
	line({{"و", 39.40, 39.75}, {"صبح", 39.85, 40.60}, {"همیشه", 40.70, 41.60},
		  {"زود", 41.70, 42.35}, {"رسید", 42.45, 43.80}});

	line({{"too", 44.20, 44.75}, {"soon", 44.85, 46.10}, {"for", 46.20, 46.60},
		  {"us", 46.70, 47.90}});
	line({{"so", 48.30, 48.75}, {"we", 48.85, 49.20}, {"sang", 49.30, 50.15},
		  {"until", 50.25, 51.10}, {"the", 51.20, 51.55}, {"sky", 51.65, 53.00}});
	line({{"turned", 53.40, 54.15}, {"into", 54.25, 54.85}, {"a", 54.95, 55.25},
		  {"colour", 55.35, 56.30}, {"we", 56.40, 56.75}, {"knew", 56.85, 58.30}});
	line({{"I", 58.70, 59.05}, {"still", 59.15, 59.90}, {"remember", 60.00, 61.15},
		  {"when", 61.25, 62.80}});

	m_bulkLoading = false;
	notifyLinesChanged();
=======
    // Every addWord() would otherwise fan out to linesChanged(), and the QML
    // Repeater would rebuild every delegate once per word. Load silently and
    // announce the finished model once at the end.
    m_bulkLoading = true;
    clear();

    const auto line = [this](std::initializer_list<DemoWord> words) {
        LyricLine *l = addLine();
        for (const DemoWord &word : words)
            l->addWord(QString::fromUtf8(word.text), word.start, word.end);
    };

    line({{"I", 0.60, 0.90}, {"remember", 1.00, 2.05}, {"when", 2.20, 3.60}});
    line({{"we", 4.00, 4.30}, {"used", 4.40, 5.10}, {"to", 5.20, 5.55}, {"stay", 5.70, 7.00}});
    line({{"under", 7.45, 8.15}, {"the", 8.25, 8.65}, {"summer", 8.80, 10.40}});
    line({{"sky", 10.85, 11.45}, {"together", 11.60, 13.60}});
    line({{"تست", 13.60, 14.00}, {"فارسی", 14.00, 15.50}});

    line({{"counting", 15.90, 16.55}, {"every", 16.65, 17.20},
          {"little", 17.30, 17.85}, {"thing", 17.95, 19.30}});
    line({{"we", 19.70, 20.05}, {"promised", 20.15, 21.00}, {"not", 21.10, 21.45},
          {"to", 21.55, 21.85}, {"forget", 21.95, 23.30}});
    line({{"the", 23.70, 24.00}, {"streetlights", 24.10, 25.25}, {"hummed", 25.35, 26.30}});
    line({{"our", 26.70, 27.10}, {"names", 27.20, 28.30}, {"in", 28.40, 28.75},
          {"the", 28.85, 29.15}, {"dark", 29.25, 30.60}});
    line({{"and", 31.00, 31.40}, {"no", 31.50, 31.90}, {"one", 32.00, 32.55},
          {"ever", 32.65, 33.40}, {"asked", 33.50, 34.80}});
    line({{"why", 35.20, 35.70}, {"we", 35.80, 36.15}, {"stayed", 36.25, 37.10},
          {"so", 37.20, 37.60}, {"late", 37.70, 39.00}});

    // A second right-to-left line, further down the list, so scrolling and
    // bidi shaping are tested together.
    line({{"و", 39.40, 39.75}, {"صبح", 39.85, 40.60}, {"همیشه", 40.70, 41.60},
          {"زود", 41.70, 42.35}, {"رسید", 42.45, 43.80}});

    line({{"too", 44.20, 44.75}, {"soon", 44.85, 46.10}, {"for", 46.20, 46.60},
          {"us", 46.70, 47.90}});
    line({{"so", 48.30, 48.75}, {"we", 48.85, 49.20}, {"sang", 49.30, 50.15},
          {"until", 50.25, 51.10}, {"the", 51.20, 51.55}, {"sky", 51.65, 53.00}});
    line({{"turned", 53.40, 54.15}, {"into", 54.25, 54.85}, {"a", 54.95, 55.25},
          {"colour", 55.35, 56.30}, {"we", 56.40, 56.75}, {"knew", 56.85, 58.30}});
    line({{"I", 58.70, 59.05}, {"still", 59.15, 59.90}, {"remember", 60.00, 61.15},
          {"when", 61.25, 62.80}});

    m_bulkLoading = false;
    notifyLinesChanged();
>>>>>>> a1707b0b544d18feaf8ddd09a460fcebdba42648
}
