#include "lyrics.h"

QString LyricLine::text() const
{
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
}

qreal LyricLine::end() const
{
	return m_words.isEmpty() ? 0.0 : m_words.last()->end();
}

Word *LyricLine::addWord(const QString &text, qreal start, qreal end)
{
	auto *word = new Word(this);
	word->setText(text);
	word->setStart(start);
	word->setEnd(end);
	m_words.append(word);
	emit wordsChanged();
	return word;
}

void LyricLine::clear()
{
	qDeleteAll(m_words);
	m_words.clear();
	emit wordsChanged();
}
