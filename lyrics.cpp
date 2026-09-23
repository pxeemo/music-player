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

qreal LyricsModel::duration() const
{
    return m_lines.isEmpty() ? 0.0 : m_lines.last()->end();
}

LyricLine *LyricsModel::lineAt(int index) const
{
    if (index < 0 || index >= m_lines.size())
        return nullptr;
    return m_lines.at(index);
}

LyricLine *LyricsModel::addLine()
{
    auto *line = new LyricLine(this);
    // A line's start/end are derived from its words, and so is duration() above.
    // Without this forward, QML bindings that read duration would never learn
    // that words arrived after the line itself was appended.
    connect(line, &LyricLine::wordsChanged, this, &LyricsModel::linesChanged);
    m_lines.append(line);
    emit linesChanged();
    return line;
}

void LyricsModel::clear()
{
    qDeleteAll(m_lines);
    m_lines.clear();
    emit linesChanged();
}

int LyricsModel::activeLine(qreal position) const
{
    int active = -1;
    for (int i = 0; i < m_lines.size(); ++i) {
        if (m_lines.at(i)->start() <= position)
            active = i;
        else
            break;
    }
    return active;
}

// Hard-coded demo timings (~13.6 s of lyrics). The shape of this data is the
// contract with the renderer; a parser would produce the same thing.
void LyricsModel::loadDemoLyrics()
{
    clear();

    LyricLine *line = addLine();
    line->addWord("I", 0.60, 0.90);
    line->addWord("remember", 1.00, 2.05);
    line->addWord("when", 2.20, 3.60);

    line = addLine();
    line->addWord("we", 4.00, 4.30);
    line->addWord("used", 4.40, 5.10);
    line->addWord("to", 5.20, 5.55);
    line->addWord("stay", 5.70, 7.00);

    line = addLine();
    line->addWord("under", 7.45, 8.15);
    line->addWord("the", 8.25, 8.65);
    line->addWord("summer", 8.80, 10.40);

    line = addLine();
    line->addWord("sky", 10.85, 11.45);
    line->addWord("together", 11.60, 13.60);

    line = addLine();
    line->addWord("تست", 13.60, 14.00);
    line->addWord("فارسی", 14.00, 15.50);
}
