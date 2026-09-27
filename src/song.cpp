#include "song.h"

#include <initializer_list>

// ---------------------------------------------------------------------------
// Song
// ---------------------------------------------------------------------------

void Song::setTitle(const QString &title)
{
	if (m_title == title)
		return;
	m_title = title;
	emit titleChanged();
}

void Song::setArtist(const QString &artist)
{
	if (m_artist == artist)
		return;
	m_artist = artist;
	emit artistChanged();
}

void Song::setColorA(const QColor &color)
{
	if (m_colorA == color)
		return;
	m_colorA = color;
	emit colorAChanged();
}

void Song::setColorB(const QColor &color)
{
	if (m_colorB == color)
		return;
	m_colorB = color;
	emit colorBChanged();
}

void Song::notifyLyricsChanged()
{
	if (!m_bulkLoading)
		emit lyricsChanged();
}

qreal Song::duration() const
{
	return m_lyrics.isEmpty() ? 0.0 : m_lyrics.last()->end();
}

LyricLine *Song::lineAt(int index) const
{
	if (index < 0 || index >= m_lyrics.size())
		return nullptr;
	return m_lyrics.at(index);
}

LyricLine *Song::addLine()
{
	auto *line = new LyricLine(this);
	// A line's start/end derive from its words, and so does duration() above.
	// Forward the line's own changes so QML bindings that read the song's
	// duration learn that words arrived after the line was appended.
	connect(line, &LyricLine::wordsChanged, this, &Song::notifyLyricsChanged);
	m_lyrics.append(line);
	notifyLyricsChanged();
	return line;
}

void Song::clear()
{
	qDeleteAll(m_lyrics);
	m_lyrics.clear();
	notifyLyricsChanged();
}

int Song::activeLine(qreal position) const
{
	int active = -1;
	for (int i = 0; i < m_lyrics.size(); ++i) {
		if (m_lyrics.at(i)->start() <= position)
			active = i;
		else
			break;
	}
	return active;
}

void Song::beginLoad()
{
	m_bulkLoading = true;
}

void Song::endLoad()
{
	m_bulkLoading = false;
	notifyLyricsChanged();
}

// ---------------------------------------------------------------------------
// SongsModel
// ---------------------------------------------------------------------------

Song *SongsModel::songAt(int index) const
{
	if (index < 0 || index >= m_songs.size())
		return nullptr;
	return m_songs.at(index);
}

Song *SongsModel::addSong()
{
	auto *song = new Song(this);
	m_songs.append(song);
	emit songsChanged();
	return song;
}

void SongsModel::clear()
{
	qDeleteAll(m_songs);
	m_songs.clear();
	emit songsChanged();
}

// One entry of the demo data below: a word with its time range in seconds.
namespace {
struct DemoWord {
	const char *text;
	qreal start;
	qreal end;
};

using DemoLine = std::initializer_list<DemoWord>;

// Fills a song's lyrics from a list of line definitions. Loading is done in
// bulk so the QML Repeater does not rebuild once per word; the finished song is
// announced once at the end.
void loadLyrics(Song *song, std::initializer_list<DemoLine> lines)
{
	song->beginLoad();
	for (const DemoLine &words : lines) {
		LyricLine *line = song->addLine();
		for (const DemoWord &word : words)
			line->addWord(QString::fromUtf8(word.text), word.start, word.end);
	}
	song->endLoad();
}
} // namespace

// Hard-coded demo catalogue. The shape of this data is the contract with the
// renderer; a real library (files on disk, an LRC/TTML parser) would produce
// the same Song objects.
void SongsModel::loadDemoSongs()
{
	clear();

	// --- Neon Summer ------------------------------------------------------
	{
		Song *song = addSong();
		song->setTitle(QStringLiteral("Neon Summer"));
		song->setArtist(QStringLiteral("The Demo Band"));
		song->setColorA(QColor("#ff5d8f"));
		song->setColorB(QColor("#7b2ff7"));
		loadLyrics(song, {
			{{"I", 0.60, 0.90}, {"remember", 1.00, 2.05}, {"when", 2.20, 3.60}},
			{{"we", 4.00, 4.30}, {"used", 4.40, 5.10}, {"to", 5.20, 5.55}, {"stay", 5.70, 7.00}},
			{{"under", 7.45, 8.15}, {"the", 8.25, 8.65}, {"summer", 8.80, 10.40}},
			{{"sky", 10.85, 11.45}, {"together", 11.60, 13.60}},
			{{"تست", 13.60, 14.00}, {"فارسی", 14.00, 15.50}},
			{{"counting", 15.90, 16.55}, {"every", 16.65, 17.20},
			 {"little", 17.30, 17.85}, {"thing", 17.95, 19.30}},
			{{"we", 19.70, 20.05}, {"promised", 20.15, 21.00}, {"not", 21.10, 21.45},
			 {"to", 21.55, 21.85}, {"forget", 21.95, 23.30}},
			{{"the", 23.70, 24.00}, {"streetlights", 24.10, 25.25}, {"hummed", 25.35, 26.30}},
			{{"our", 26.70, 27.10}, {"names", 27.20, 28.30}, {"in", 28.40, 28.75},
			 {"the", 28.85, 29.15}, {"dark", 29.25, 30.60}},
			{{"and", 31.00, 31.40}, {"no", 31.50, 31.90}, {"one", 32.00, 32.55},
			 {"ever", 32.65, 33.40}, {"asked", 33.50, 34.80}},
			{{"why", 35.20, 35.70}, {"we", 35.80, 36.15}, {"stayed", 36.25, 37.10},
			 {"so", 37.20, 37.60}, {"late", 37.70, 39.00}},
			{{"و", 39.40, 39.75}, {"صبح", 39.85, 40.60}, {"همیشه", 40.70, 41.60},
			 {"زود", 41.70, 42.35}, {"رسید", 42.45, 43.80}},
			{{"too", 44.20, 44.75}, {"soon", 44.85, 46.10}, {"for", 46.20, 46.60},
			 {"us", 46.70, 47.90}},
			{{"so", 48.30, 48.75}, {"we", 48.85, 49.20}, {"sang", 49.30, 50.15},
			 {"until", 50.25, 51.10}, {"the", 51.20, 51.55}, {"sky", 51.65, 53.00}},
			{{"turned", 53.40, 54.15}, {"into", 54.25, 54.85}, {"a", 54.95, 55.25},
			 {"colour", 55.35, 56.30}, {"we", 56.40, 56.75}, {"knew", 56.85, 58.30}},
			{{"I", 58.70, 59.05}, {"still", 59.15, 59.90}, {"remember", 60.00, 61.15},
			 {"when", 61.25, 62.80}},
		});
	}

	// --- Midnight Drive ---------------------------------------------------
	{
		Song *song = addSong();
		song->setTitle(QStringLiteral("Midnight Drive"));
		song->setArtist(QStringLiteral("Aurora Lane"));
		song->setColorA(QColor("#2b6cff"));
		song->setColorB(QColor("#12d5c4"));
		loadLyrics(song, {
			{{"City", 0.50, 1.10}, {"lights", 1.20, 1.80}, {"are", 1.95, 2.20}, {"blurring", 2.35, 3.30}},
			{{"past", 3.75, 4.30}, {"the", 4.45, 4.70}, {"window", 4.85, 5.80}},
			{{"I", 6.25, 6.45}, {"don't", 6.60, 7.10}, {"need", 7.25, 7.70}, {"to", 7.85, 8.05},
			 {"know", 8.20, 8.65}, {"where", 8.80, 9.30}, {"we're", 9.45, 9.85}, {"going", 10.00, 11.05}},
			{{"as", 11.50, 11.75}, {"long", 11.90, 12.35}, {"as", 12.50, 12.70}, {"you're", 12.85, 13.35},
			 {"here", 13.50, 14.20}},
			{{"the", 14.65, 14.90}, {"radio", 15.05, 15.85}, {"is", 16.00, 16.25},
			 {"playing", 16.40, 17.25}, {"slow", 17.40, 18.00}},
			{{"and", 18.45, 18.75}, {"the", 18.90, 19.15}, {"night", 19.30, 20.00},
			 {"is", 20.15, 20.40}, {"ours", 20.55, 21.35}},
			{{"we", 21.80, 22.10}, {"can", 22.25, 22.60}, {"drive", 22.75, 23.40}, {"till", 23.55, 23.95},
			 {"the", 24.10, 24.35}, {"morning", 24.50, 25.60}},
			{{"and", 26.05, 26.35}, {"never", 26.50, 27.15}, {"look", 27.30, 27.80}, {"back", 27.95, 28.70}},
		});
	}

	// --- Golden Hour ------------------------------------------------------
	{
		Song *song = addSong();
		song->setTitle(QStringLiteral("Golden Hour"));
		song->setArtist(QStringLiteral("Mira Sol"));
		song->setColorA(QColor("#ffb03a"));
		song->setColorB(QColor("#ff5f6d"));
		loadLyrics(song, {
			{{"The", 0.50, 0.80}, {"sun", 0.95, 1.45}, {"is", 1.60, 1.85}, {"low", 2.00, 2.60}},
			{{"and", 3.05, 3.35}, {"the", 3.50, 3.75}, {"sky", 3.90, 4.45}, {"is", 4.60, 4.85},
			 {"honey", 5.00, 6.10}},
			{{"we", 6.55, 6.85}, {"are", 7.00, 7.35}, {"standing", 7.50, 8.45}, {"still", 8.60, 9.30}},
			{{"letting", 9.75, 10.55}, {"time", 10.70, 11.25}, {"go", 11.40, 11.90}, {"by", 12.05, 12.65}},
			{{"every", 13.10, 13.85}, {"shadow", 14.00, 14.95}, {"turns", 15.10, 15.80},
			 {"golden", 15.95, 17.15}},
			{{"and", 17.60, 17.90}, {"every", 18.05, 18.80}, {"worry", 18.95, 19.90}, {"fades", 20.05, 20.80}},
			{{"hold", 21.25, 21.80}, {"on", 21.95, 22.40}, {"to", 22.55, 22.80}, {"this", 22.95, 23.35},
			 {"moment", 23.50, 24.70}},
			{{"before", 25.15, 25.95}, {"it", 26.10, 26.45}, {"slips", 26.60, 27.35}, {"away", 27.50, 28.70}},
		});
	}

	// --- Paper Planes -----------------------------------------------------
	{
		Song *song = addSong();
		song->setTitle(QStringLiteral("Paper Planes"));
		song->setArtist(QStringLiteral("Kite & Co."));
		song->setColorA(QColor("#33d17a"));
		song->setColorB(QColor("#0f9b8e"));
		loadLyrics(song, {
			{{"We", 0.50, 0.80}, {"folded", 0.95, 1.70}, {"our", 1.85, 2.15}, {"dreams", 2.30, 3.15}},
			{{"into", 3.60, 4.25}, {"paper", 4.40, 5.25}, {"planes", 5.40, 6.30}},
			{{"threw", 6.75, 7.55}, {"them", 7.70, 8.15}, {"off", 8.30, 8.75}, {"the", 8.90, 9.15},
			 {"roof", 9.30, 10.05}},
			{{"and", 10.50, 10.80}, {"watched", 10.95, 11.80}, {"them", 11.95, 12.40},
			 {"fly", 12.55, 13.35}},
			{{"some", 13.80, 14.35}, {"got", 14.50, 14.90}, {"caught", 15.05, 15.75}, {"in", 15.90, 16.15},
			 {"the", 16.30, 16.55}, {"rain", 16.70, 17.65}},
			{{"some", 18.10, 18.65}, {"never", 18.80, 19.55}, {"came", 19.70, 20.30},
			 {"down", 20.45, 21.40}},
			{{"but", 21.85, 22.15}, {"we", 22.30, 22.60}, {"kept", 22.75, 23.30}, {"folding", 23.45, 24.60}},
			{{"until", 25.05, 25.80}, {"the", 25.95, 26.20}, {"morning", 26.35, 27.50}, {"came", 27.65, 28.60}},
		});
	}
}
