#include "song.h"

#include "lrcparser.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QRegularExpression>
#include <QStandardPaths>

#include <utility>

#include <taglib/attachedpictureframe.h>
#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4coverart.h>
#include <taglib/mp4file.h>
#include <taglib/mp4item.h>
#include <taglib/mp4tag.h>
#include <taglib/mpegfile.h>
#include <taglib/opusfile.h>
#include <taglib/tag.h>
#include <taglib/tdebuglistener.h>
#include <taglib/tpropertymap.h>
#include <taglib/unsynchronizedlyricsframe.h>
#include <taglib/vorbisfile.h>
#include <taglib/xiphcomment.h>

namespace {

// TagLib chatters on stderr for files with slightly unusual tags. Quiet it: the
// library is expected to contain some odd rips.
class SilentTagLibListener : public TagLib::DebugListener {
  public:
    void printMessage(const TagLib::String &) override {}
};

QString fromTagLib(const TagLib::String &value)
{
	// to8Bit(true) gives UTF-8, which is what TagLib stores for modern tags.
	return QString::fromStdString(value.to8Bit(true));
}

// Unsynced lyrics living in an ID3v2 USLT frame (MP3, and some FLAC/WAV files).
QString id3Lyrics(TagLib::ID3v2::Tag *id3)
{
	if (!id3)
		return {};
	const TagLib::ID3v2::FrameList frames = id3->frameList("USLT");
	for (const auto *frame : frames) {
		if (const auto *uslt =
				dynamic_cast<const TagLib::ID3v2::UnsynchronizedLyricsFrame *>(frame))
			return fromTagLib(uslt->text());
	}
	return {};
}

// Lyrics in a Vorbis comment (FLAC, Ogg Vorbis, Opus).
QString xiphLyrics(TagLib::Ogg::XiphComment *xiph)
{
	if (!xiph)
		return {};
	for (const char *key : {"LYRICS", "UNSYNCEDLYRICS", "UNSYNCED LYRICS"}) {
		const TagLib::StringList values = xiph->fieldListMap().value(TagLib::String(key));
		if (!values.isEmpty())
			return fromTagLib(values.front());
	}
	return {};
}

QString embeddedLyrics(TagLib::File *file)
{
	if (!file)
		return {};

	if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(file)) {
		if (QString text = id3Lyrics(mpeg->ID3v2Tag()); !text.isEmpty())
			return text;
	}
	if (auto *flac = dynamic_cast<TagLib::FLAC::File *>(file)) {
		if (QString text = xiphLyrics(flac->xiphComment()); !text.isEmpty())
			return text;
		if (QString text = id3Lyrics(flac->ID3v2Tag()); !text.isEmpty())
			return text;
	}
	if (auto *vorbis = dynamic_cast<TagLib::Ogg::Vorbis::File *>(file)) {
		if (QString text = xiphLyrics(vorbis->tag()); !text.isEmpty())
			return text;
	}
	if (auto *opus = dynamic_cast<TagLib::Ogg::Opus::File *>(file)) {
		if (QString text = xiphLyrics(opus->tag()); !text.isEmpty())
			return text;
	}
	if (auto *mp4 = dynamic_cast<TagLib::MP4::File *>(file)) {
		if (auto *tag = mp4->tag()) {
			// The MP4 lyrics atom key is the Latin-1 bytes 0xA9 'l' 'y' 'r'.
			const TagLib::MP4::Item item =
				tag->item(TagLib::String("\xA9lyr", TagLib::String::Latin1));
			if (item.isValid()) {
				const TagLib::StringList values = item.toStringList();
				if (!values.isEmpty())
					return fromTagLib(values.front());
			}
		}
	}

	// Last resort: formats that expose lyrics through the generic property map.
	const TagLib::PropertyMap map = file->properties();
	for (const char *key : {"LYRICS", "UNSYNCEDLYRICS"}) {
		const auto it = map.find(key);
		if (it != map.end() && !it->second.isEmpty())
			return fromTagLib(it->second.front());
	}
	return {};
}

// Cover art pulled out of a tag, plus its mime type when the tag knows one.
struct EmbeddedArtwork {
	QByteArray data;
	QString mime;
};

QString mimeFromMp4(TagLib::MP4::CoverArt::Format format)
{
	switch (format) {
	case TagLib::MP4::CoverArt::JPEG:
		return QStringLiteral("image/jpeg");
	case TagLib::MP4::CoverArt::PNG:
		return QStringLiteral("image/png");
	case TagLib::MP4::CoverArt::BMP:
		return QStringLiteral("image/bmp");
	case TagLib::MP4::CoverArt::GIF:
		return QStringLiteral("image/gif");
	default:
		return {};
	}
}

// Pictures in an Ogg/Opus Vorbis comment are a base64-encoded FLAC picture
// block. Decode it and let TagLib parse the block for us.
EmbeddedArtwork xiphArtwork(TagLib::Ogg::XiphComment *comment)
{
	if (!comment)
		return {};
	const TagLib::StringList values =
		comment->fieldListMap().value(TagLib::String("METADATA_BLOCK_PICTURE"));
	if (values.isEmpty())
		return {};
	const QByteArray decoded =
		QByteArray::fromBase64(QByteArray::fromStdString(values.front().to8Bit(true)));
	if (decoded.isEmpty())
		return {};
	const TagLib::FLAC::Picture picture(TagLib::ByteVector(decoded.constData(), unsigned(decoded.size())));
	const TagLib::ByteVector data = picture.data();
	if (data.isEmpty())
		return {};
	return {QByteArray(data.data(), int(data.size())), fromTagLib(picture.mimeType())};
}

// The first embedded cover picture in the file, if any.
EmbeddedArtwork embeddedArtwork(TagLib::File *file)
{
	if (!file)
		return {};

	const auto fromApic = [](TagLib::ID3v2::Tag *id3) -> EmbeddedArtwork {
		if (!id3)
			return {};
		const TagLib::ID3v2::FrameList frames = id3->frameList("APIC");
		for (const auto *frame : frames) {
			if (const auto *picture =
					dynamic_cast<const TagLib::ID3v2::AttachedPictureFrame *>(frame)) {
				const TagLib::ByteVector data = picture->picture();
				if (!data.isEmpty())
					return {QByteArray(data.data(), int(data.size())),
							fromTagLib(picture->mimeType())};
			}
		}
		return {};
	};

	if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(file)) {
		if (EmbeddedArtwork art = fromApic(mpeg->ID3v2Tag()); !art.data.isEmpty())
			return art;
	}
	if (auto *flac = dynamic_cast<TagLib::FLAC::File *>(file)) {
		const auto pictures = flac->pictureList();
		if (!pictures.isEmpty()) {
			const TagLib::FLAC::Picture *picture = pictures.front();
			const TagLib::ByteVector data = picture->data();
			if (!data.isEmpty())
				return {QByteArray(data.data(), int(data.size())), fromTagLib(picture->mimeType())};
		}
		if (EmbeddedArtwork art = fromApic(flac->ID3v2Tag()); !art.data.isEmpty())
			return art;
	}
	if (auto *vorbis = dynamic_cast<TagLib::Ogg::Vorbis::File *>(file)) {
		if (EmbeddedArtwork art = xiphArtwork(vorbis->tag()); !art.data.isEmpty())
			return art;
	}
	if (auto *opus = dynamic_cast<TagLib::Ogg::Opus::File *>(file)) {
		if (EmbeddedArtwork art = xiphArtwork(opus->tag()); !art.data.isEmpty())
			return art;
	}
	if (auto *mp4 = dynamic_cast<TagLib::MP4::File *>(file)) {
		if (auto *tag = mp4->tag()) {
			const TagLib::MP4::Item item = tag->item("covr");
			if (item.isValid()) {
				const auto covers = item.toCoverArtList();
				if (!covers.isEmpty()) {
					const TagLib::MP4::CoverArt &cover = covers.front();
					const TagLib::ByteVector data = cover.data();
					if (!data.isEmpty())
						return {QByteArray(data.data(), int(data.size())),
								mimeFromMp4(cover.format())};
				}
			}
		}
	}
	return {};
}

QString extensionForMime(const QString &mime)
{
	const QString lowered = mime.toLower();
	if (lowered.contains(QLatin1String("jpeg")) || lowered.contains(QLatin1String("jpg")))
		return QStringLiteral(".jpg");
	if (lowered.contains(QLatin1String("png")))
		return QStringLiteral(".png");
	if (lowered.contains(QLatin1String("webp")))
		return QStringLiteral(".webp");
	if (lowered.contains(QLatin1String("gif")))
		return QStringLiteral(".gif");
	if (lowered.contains(QLatin1String("bmp")))
		return QStringLiteral(".bmp");
	return {};
}

// When the tag gives no mime type, let Qt look at the bytes.
QString sniffImageExtension(const QByteArray &bytes)
{
	QBuffer buffer;
	buffer.setData(bytes);
	if (!buffer.open(QIODevice::ReadOnly))
		return {};
	QImageReader reader(&buffer);
	reader.setDecideFormatFromContent(true);
	const QString format = QString::fromLatin1(reader.format()).toLower();
	if (format == QLatin1String("jpeg") || format == QLatin1String("jpg"))
		return QStringLiteral(".jpg");
	if (!format.isEmpty())
		return QLatin1Char('.') + format;
	return {};
}

// Writes the cover into the app cache and returns a file URL for the UI. Files
// are named by content hash, so every track on an album shares one cover.
QUrl cacheArtwork(const EmbeddedArtwork &artwork)
{
	if (artwork.data.isEmpty())
		return {};

	QString extension = extensionForMime(artwork.mime);
	if (extension.isEmpty())
		extension = sniffImageExtension(artwork.data);
	if (extension.isEmpty())
		return {};

	const QString hash = QString::fromLatin1(
		QCryptographicHash::hash(artwork.data, QCryptographicHash::Sha1).toHex());
	QString directory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	if (directory.isEmpty())
		directory = QDir::tempPath() + QStringLiteral("/karaoke");
	directory += QStringLiteral("/artwork");
	QDir().mkpath(directory);

	const QString path = directory + QLatin1Char('/') + hash + extension;
	if (!QFile::exists(path)) {
		QFile file(path);
		if (!file.open(QIODevice::WriteOnly))
			return {};
		file.write(artwork.data);
	}
	return QUrl::fromLocalFile(path);
}

// A sidecar lyric file next to the audio, if there is one.
QString sidecarPath(const QString &audioPath)
{
	const QFileInfo info(audioPath);
	const QString base = info.absolutePath() + QLatin1Char('/') + info.completeBaseName();
	for (const char *ext : {".lrc", ".LRC", ".ttml", ".TTML"}) {
		const QString path = base + QLatin1String(ext);
		if (QFile::exists(path))
			return path;
	}
	return {};
}

// Turns a raw .lrc/.ttml file into something worth reading, without attempting
// real timing. This is deliberately crude: a full parser is a later phase.
QString plainLyrics(const QString &raw, bool ttml)
{
	if (ttml) {
		QString text = raw;
		const auto ci = QRegularExpression::CaseInsensitiveOption;
		text.replace(QRegularExpression(QStringLiteral("<\\s*br\\s*/?>"), ci),
					 QStringLiteral("\n"));
		text.replace(QRegularExpression(QStringLiteral("<\\s*/\\s*(p|div|text)\\s*>"), ci),
					 QStringLiteral("\n"));
		text.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
		text.replace(QLatin1String("&amp;"), QLatin1String("&"));
		text.replace(QLatin1String("&lt;"), QLatin1String("<"));
		text.replace(QLatin1String("&gt;"), QLatin1String(">"));
		text.replace(QLatin1String("&quot;"), QLatin1String("\""));
		text.replace(QLatin1String("&apos;"), QLatin1String("'"));
		text.replace(QLatin1String("&#39;"), QLatin1String("'"));
		return text.trimmed();
	}

	// LRC: drop the [mm:ss.xx] and [ar:...] tags, keep the words.
	QStringList lines;
	const QStringList rawLines = raw.split(QLatin1Char('\n'));
	const QRegularExpression tag(QStringLiteral("^\\s*(\\[[^\\]]*\\])+\\s*"));
	for (QString line : rawLines) {
		line.remove(tag);
		line = line.trimmed();
		if (!line.isEmpty())
			lines.append(line);
	}
	return lines.join(QLatin1Char('\n'));
}

// "Artist - Title.mp3" when the tags are missing.
void titleFromFileName(const QFileInfo &info, QString *title, QString *artist)
{
	const QString base = info.completeBaseName();
	const int dash = base.indexOf(QStringLiteral(" - "));
	if (dash > 0) {
		*artist = base.left(dash).trimmed();
		*title = base.mid(dash + 3).trimmed();
	} else {
		*title = base.trimmed();
	}
}

// Embedded lyrics have no file extension, so the format is guessed from the
// text: TTML is XML and starts with '<'; LRC carries `[mm:ss.xx]` markers.
bool looksLikeTtml(const QString &text)
{
	return text.trimmed().startsWith(QLatin1Char('<'));
}

// Fallback for lyric sources no parser handles yet (TTML, or untimed embedded
// text): present the extracted plain text as one untimed lyric line per row, so
// the view still shows something.
lyrics::Lyrics fallbackDocument(const QString &plain, const QString &title, const QString &artist)
{
	lyrics::Lyrics document;
	document.metadata().title = title;
	document.metadata().artist = artist;
	const QStringList rows = plain.split(QLatin1Char('\n'));
	for (const QString &row : rows) {
		const QString trimmed = row.trimmed();
		if (trimmed.isEmpty())
			continue;
		document.addLine().mainVocal.text = trimmed;
	}
	return document;
}

void loadSongFromFile(const QString &path, Song *song)
{
	song->setFilePath(path);

	const QFileInfo info(path);
	TagLib::FileRef ref(QFile::encodeName(path).constData());

	QString title;
	QString artist;
	QString album;
	qreal duration = 0.0;
	QString embedded;
	QUrl artwork;

	if (!ref.isNull()) {
		if (const TagLib::Tag *tag = ref.tag()) {
			title = fromTagLib(tag->title()).trimmed();
			artist = fromTagLib(tag->artist()).trimmed();
			album = fromTagLib(tag->album()).trimmed();
		}
		if (const TagLib::AudioProperties *props = ref.audioProperties())
			duration = props->lengthInMilliseconds() / 1000.0;
		embedded = embeddedLyrics(ref.file());
		artwork = cacheArtwork(embeddedArtwork(ref.file()));
	}

	if (title.isEmpty() || artist.isEmpty()) {
		QString fileTitle;
		QString fileArtist;
		titleFromFileName(info, &fileTitle, &fileArtist);
		if (title.isEmpty())
			title = fileTitle;
		if (artist.isEmpty())
			artist = fileArtist;
	}

	// Collect the raw lyric source. A sidecar file wins over the embedded tag,
	// and its extension names the format; embedded lyrics have no extension, so
	// the format is guessed from the text itself.
	QString rawLyrics = embedded;
	bool isTtml = looksLikeTtml(embedded);
	bool isLrc = !isTtml && lyrics::LrcParser::looksLikeLrc(embedded);

	const QString sidecar = sidecarPath(path);
	if (!sidecar.isEmpty()) {
		QFile file(sidecar);
		if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			const QString contents = QString::fromUtf8(file.readAll());
			if (!contents.trimmed().isEmpty()) {
				rawLyrics = contents;
				isTtml = sidecar.endsWith(QLatin1String("ttml"), Qt::CaseInsensitive);
				isLrc = !isTtml && (sidecar.endsWith(QLatin1String("lrc"), Qt::CaseInsensitive)
									|| lyrics::LrcParser::looksLikeLrc(contents));
			}
		}
	}

	// Parse when a parser matches, otherwise fall back to plain extracted text.
	lyrics::Lyrics document;
	bool parsed = false;
	if (isLrc) {
		const lyrics::LrcParser parser;
		lyrics::ParseResult result = parser.parse(rawLyrics.toUtf8());
		if (result.succeeded()) {
			document = std::move(*result.lyrics);
			parsed = true;
		}
	}
	if (!parsed)
		document = fallbackDocument(plainLyrics(rawLyrics, isTtml), title, artist);
	if (document.metadata().title.isEmpty())
		document.metadata().title = title;
	if (document.metadata().artist.isEmpty())
		document.metadata().artist = artist;

	song->setTitle(title);
	song->setArtist(artist);
	song->setAlbum(album);
	song->setDuration(duration);
	song->setLyrics(plainLyrics(rawLyrics, isTtml).trimmed());
	song->setArtworkSource(artwork);
	song->setLyricsDocument(std::move(document));
}

} // namespace

// ---------------------------------------------------------------------------
// Song
// ---------------------------------------------------------------------------

Song::Song(QObject *parent) : QObject(parent), m_document(new LyricsDocument(this)) {}

void Song::setLyricsDocument(lyrics::Lyrics &&document)
{
	m_document->setLyrics(std::move(document));
}

void Song::setFilePath(const QString &path)
{
	if (m_filePath == path)
		return;
	m_filePath = path;
	emit filePathChanged();
}

void Song::setTitle(const QString &title)
{
	if (m_title == title)
		return;
	m_title = title;
	emit titleChanged();
	emit colorsChanged();
}

void Song::setArtist(const QString &artist)
{
	if (m_artist == artist)
		return;
	m_artist = artist;
	emit artistChanged();
	emit colorsChanged();
}

void Song::setAlbum(const QString &album)
{
	if (m_album == album)
		return;
	m_album = album;
	emit albumChanged();
}

void Song::setDuration(qreal duration)
{
	if (qFuzzyCompare(m_duration, duration))
		return;
	m_duration = duration;
	emit durationChanged();
}

void Song::setLyrics(const QString &lyrics)
{
	if (m_lyrics == lyrics)
		return;
	m_lyrics = lyrics;
	emit lyricsChanged();
}

void Song::setArtworkSource(const QUrl &source)
{
	if (m_artworkSource == source)
		return;
	m_artworkSource = source;
	emit artworkChanged();
}

QColor Song::colorA() const
{
	const uint hash = qHash(m_title + m_artist);
	return QColor::fromHsv(int(hash % 360), 150, 225);
}

QColor Song::colorB() const
{
	const uint hash = qHash(m_artist + m_title);
	return QColor::fromHsv(int((hash / 360 + 45) % 360), 190, 120);
}

// ---------------------------------------------------------------------------
// MusicLibrary
// ---------------------------------------------------------------------------

MusicLibrary *MusicLibrary::s_instance = nullptr;

MusicLibrary::MusicLibrary(QObject *parent) : QObject(parent)
{
	s_instance = this;

	static SilentTagLibListener listener;
	TagLib::setDebugListener(&listener);

	m_timer.setInterval(0);
	connect(&m_timer, &QTimer::timeout, this, &MusicLibrary::processChunk);
}

void MusicLibrary::scan()
{
	startScan();
}

Song *MusicLibrary::songAt(int index) const
{
	if (index < 0 || index >= m_songs.size())
		return nullptr;
	return m_songs.at(index);
}

void MusicLibrary::startScan()
{
	if (m_scanning)
		return;

	qDeleteAll(m_songs);
	m_songs.clear();
	m_pending.clear();
	m_scanned = 0;
	emit songsChanged();

	QString folder = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
	if (folder.isEmpty() || !QDir(folder).exists()) {
		const QString home = QDir::homePath();
		for (const QString &candidate : {home + "/mus", home + "/Music"}) {
			if (QDir(candidate).exists()) {
				folder = candidate;
				break;
			}
		}
	}
	m_folder = folder;
	emit folderChanged();

	if (folder.isEmpty()) {
		m_total = 0;
		emit progressChanged();
		return;
	}

	const QStringList filters{"*.mp3", "*.MP3",  "*.flac", "*.FLAC", "*.ogg", "*.OGG",
							  "*.opus", "*.OPUS", "*.m4a",  "*.M4A",  "*.wav", "*.WAV",
							  "*.aac",  "*.AAC"};
	QDirIterator it(folder, filters, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
	while (it.hasNext())
		m_pending.append(it.next());

	m_total = m_pending.size();
	m_scanning = true;
	emit progressChanged();
	emit scanningChanged();

	if (m_pending.isEmpty()) {
		m_scanning = false;
		emit scanningChanged();
		return;
	}
	m_timer.start();
}

void MusicLibrary::processChunk()
{
	// A few files per tick keeps the UI responsive while the list fills in.
	constexpr int chunkSize = 24;
	int processed = 0;
	while (!m_pending.isEmpty() && processed < chunkSize) {
		const QString path = m_pending.takeFirst();
		auto *song = new Song(this);
		loadSongFromFile(path, song);
		m_songs.append(song);
		++m_scanned;
		++processed;
	}

	emit progressChanged();

	if (m_pending.isEmpty()) {
		m_timer.stop();
		m_scanning = false;
		emit songsChanged();
		emit scanningChanged();
	} else if (m_scanned % 240 == 0) {
		// Publish periodically so the list grows while scanning, without
		// rebuilding it once per file.
		emit songsChanged();
	}
}
