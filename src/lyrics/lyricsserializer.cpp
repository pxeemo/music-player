#include "lyricsserializer.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>

#include <utility>
#include <vector>

namespace lyrics {

namespace {

// Bumped whenever the layout below changes: a document written by another
// version of this code is not read, and the file it came from is read again.
constexpr int JsonVersion = 1;

// ---------------------------------------------------------------------------
// Writing. Keys are always written, even when empty, so a document that reads
// back is byte for byte the one that was stored.
// ---------------------------------------------------------------------------

QJsonObject timingJson(const OptionalTiming &timing)
{
    QJsonObject object;
    object.insert(QStringLiteral("start"), static_cast<qint64>(timing->start.count()));
    if (timing->end)
        object.insert(QStringLiteral("end"), static_cast<qint64>(timing->end->count()));
    return object;
}

QJsonObject syllableJson(const Syllable &syllable)
{
    QJsonObject object;
    object.insert(QStringLiteral("text"), syllable.text);
    if (syllable.timing)
        object.insert(QStringLiteral("timing"), timingJson(*syllable.timing));
    return object;
}

QJsonObject wordJson(const Word &word)
{
    QJsonObject object;
    object.insert(QStringLiteral("text"), word.text);
    if (word.timing)
        object.insert(QStringLiteral("timing"), timingJson(*word.timing));
    if (!word.syllables.empty()) {
        QJsonArray syllables;
        for (const Syllable &syllable : word.syllables)
            syllables.append(syllableJson(syllable));
        object.insert(QStringLiteral("syllables"), syllables);
    }
    return object;
}

QJsonObject vocalJson(const Vocal &vocal)
{
    QJsonObject object;
    object.insert(QStringLiteral("text"), vocal.text);
    if (vocal.timing)
        object.insert(QStringLiteral("timing"), timingJson(*vocal.timing));
    if (vocal.agentId != InvalidId)
        object.insert(QStringLiteral("agentId"), static_cast<qint64>(vocal.agentId));
    if (!vocal.words.empty()) {
        QJsonArray words;
        for (const Word &word : vocal.words)
            words.append(wordJson(word));
        object.insert(QStringLiteral("words"), words);
    }
    return object;
}

QString agentTypeName(AgentType type)
{
    switch (type) {
    case AgentType::Person:
        return QStringLiteral("person");
    case AgentType::Group:
        return QStringLiteral("group");
    case AgentType::Other:
        break;
    }
    return QStringLiteral("other");
}

QJsonObject lineJson(const Line &line)
{
    QJsonObject object;
    object.insert(QStringLiteral("main"), vocalJson(line.mainVocal));
    if (!line.backgrounds.empty()) {
        QJsonArray backgrounds;
        for (const Vocal &background : line.backgrounds)
            backgrounds.append(vocalJson(background));
        object.insert(QStringLiteral("backgrounds"), backgrounds);
    }
    if (!line.translations.empty()) {
        QJsonArray translations;
        for (const Translation &translation : line.translations) {
            QJsonObject item;
            item.insert(QStringLiteral("language"), translation.language);
            item.insert(QStringLiteral("text"), translation.text);
            if (translation.sourceLineId != InvalidId)
                item.insert(QStringLiteral("sourceLineId"),
                            static_cast<qint64>(translation.sourceLineId));
            translations.append(item);
        }
        object.insert(QStringLiteral("translations"), translations);
    }
    if (!line.transliterations.empty()) {
        QJsonArray transliterations;
        for (const Transliteration &transliteration : line.transliterations) {
            QJsonObject item;
            item.insert(QStringLiteral("text"), transliteration.text);
            if (transliteration.sourceId != InvalidId)
                item.insert(QStringLiteral("sourceId"),
                            static_cast<qint64>(transliteration.sourceId));
            transliterations.append(item);
        }
        object.insert(QStringLiteral("transliterations"), transliterations);
    }
    return object;
}

QJsonObject elementJson(const Element &element)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), static_cast<qint64>(element.id()));
    switch (element.kind()) {
    case ElementKind::Line: {
        const Line &line = static_cast<const Line &>(element);
        object.insert(QStringLiteral("kind"), QStringLiteral("line"));
        const QJsonObject body = lineJson(line);
        for (auto it = body.begin(); it != body.end(); ++it)
            object.insert(it.key(), it.value());
        break;
    }
    case ElementKind::Section: {
        const Section &section = static_cast<const Section &>(element);
        object.insert(QStringLiteral("kind"), QStringLiteral("section"));
        object.insert(QStringLiteral("name"), section.name);
        if (section.timing)
            object.insert(QStringLiteral("timing"), timingJson(*section.timing));
        break;
    }
    case ElementKind::Instrumental: {
        const Instrumental &instrumental = static_cast<const Instrumental &>(element);
        object.insert(QStringLiteral("kind"), QStringLiteral("instrumental"));
        object.insert(QStringLiteral("description"), instrumental.description);
        if (instrumental.timing)
            object.insert(QStringLiteral("timing"), timingJson(*instrumental.timing));
        break;
    }
    }
    return object;
}

// ---------------------------------------------------------------------------
// Reading. The cache is written by this same program, so every field it writes
// is required to come back: a missing key or a wrong type fails the document,
// which is then read from its source file again instead of being half-loaded.
// ---------------------------------------------------------------------------

bool readString(const QJsonObject &object, const QString &key, QString *out)
{
    const QJsonValue value = object.value(key);
    if (!value.isString())
        return false;
    *out = value.toString();
    return true;
}

bool readInt(const QJsonObject &object, const QString &key, qint64 *out)
{
    const QJsonValue value = object.value(key);
    if (!value.isDouble())
        return false;
    *out = static_cast<qint64>(value.toDouble());
    return true;
}

bool readArray(const QJsonObject &object, const QString &key, QJsonArray *out)
{
    const QJsonValue value = object.value(key);
    if (!value.isArray())
        return false;
    *out = value.toArray();
    return true;
}

bool readTiming(const QJsonValue &value, OptionalTiming *out)
{
    if (!value.isObject())
        return false;
    const QJsonObject object = value.toObject();
    qint64 start = -1;
    if (!readInt(object, QStringLiteral("start"), &start) || start < 0)
        return false;

    Timing timing;
    timing.start = Timestamp(start);
    if (object.contains(QStringLiteral("end"))) {
        qint64 end = -1;
        if (!readInt(object, QStringLiteral("end"), &end) || end < start)
            return false;
        timing.end = Timestamp(end);
    }
    *out = std::move(timing);
    return true;
}

bool readOptionalTiming(const QJsonObject &object, const QString &key, OptionalTiming *out)
{
    return !object.contains(key) || readTiming(object.value(key), out);
}

bool readSyllables(const QJsonObject &object, std::vector<Syllable> *out)
{
    if (!object.contains(QStringLiteral("syllables")))
        return true;
    QJsonArray array;
    if (!readArray(object, QStringLiteral("syllables"), &array))
        return false;
    out->reserve(static_cast<std::size_t>(array.size()));
    for (const QJsonValue &value : array) {
        if (!value.isObject())
            return false;
        Syllable syllable;
        const QJsonObject item = value.toObject();
        if (!readString(item, QStringLiteral("text"), &syllable.text) ||
            !readOptionalTiming(item, QStringLiteral("timing"), &syllable.timing))
            return false;
        out->push_back(std::move(syllable));
    }
    return true;
}

bool readWords(const QJsonObject &object, std::vector<Word> *out)
{
    if (!object.contains(QStringLiteral("words")))
        return true;
    QJsonArray array;
    if (!readArray(object, QStringLiteral("words"), &array))
        return false;
    out->reserve(static_cast<std::size_t>(array.size()));
    for (const QJsonValue &value : array) {
        if (!value.isObject())
            return false;
        Word word;
        const QJsonObject item = value.toObject();
        if (!readString(item, QStringLiteral("text"), &word.text) ||
            !readOptionalTiming(item, QStringLiteral("timing"), &word.timing) ||
            !readSyllables(item, &word.syllables))
            return false;
        out->push_back(std::move(word));
    }
    return true;
}

bool readVocal(const QJsonObject &object, Vocal *out)
{
    if (!readString(object, QStringLiteral("text"), &out->text) ||
        !readOptionalTiming(object, QStringLiteral("timing"), &out->timing))
        return false;

    if (object.contains(QStringLiteral("agentId"))) {
        qint64 agentId = InvalidId;
        if (!readInt(object, QStringLiteral("agentId"), &agentId) || agentId < 0)
            return false;
        out->agentId = static_cast<Id>(agentId);
    }
    return readWords(object, &out->words);
}

bool readLine(const QJsonObject &object, Line *out)
{
    if (!object.value(QStringLiteral("main")).isObject() ||
        !readVocal(object.value(QStringLiteral("main")).toObject(), &out->mainVocal))
        return false;

    if (object.contains(QStringLiteral("backgrounds"))) {
        QJsonArray array;
        if (!readArray(object, QStringLiteral("backgrounds"), &array))
            return false;
        for (const QJsonValue &value : array) {
            if (!value.isObject())
                return false;
            Vocal background;
            if (!readVocal(value.toObject(), &background))
                return false;
            out->backgrounds.push_back(std::move(background));
        }
    }

    if (object.contains(QStringLiteral("translations"))) {
        QJsonArray array;
        if (!readArray(object, QStringLiteral("translations"), &array))
            return false;
        for (const QJsonValue &value : array) {
            if (!value.isObject())
                return false;
            Translation translation;
            const QJsonObject item = value.toObject();
            if (!readString(item, QStringLiteral("language"), &translation.language) ||
                !readString(item, QStringLiteral("text"), &translation.text))
                return false;
            if (item.contains(QStringLiteral("sourceLineId"))) {
                qint64 id = InvalidId;
                if (!readInt(item, QStringLiteral("sourceLineId"), &id) || id < 0)
                    return false;
                translation.sourceLineId = static_cast<Id>(id);
            }
            out->translations.push_back(std::move(translation));
        }
    }

    if (object.contains(QStringLiteral("transliterations"))) {
        QJsonArray array;
        if (!readArray(object, QStringLiteral("transliterations"), &array))
            return false;
        for (const QJsonValue &value : array) {
            if (!value.isObject())
                return false;
            Transliteration transliteration;
            const QJsonObject item = value.toObject();
            if (!readString(item, QStringLiteral("text"), &transliteration.text))
                return false;
            if (item.contains(QStringLiteral("sourceId"))) {
                qint64 id = InvalidId;
                if (!readInt(item, QStringLiteral("sourceId"), &id) || id < 0)
                    return false;
                transliteration.sourceId = static_cast<Id>(id);
            }
            out->transliterations.push_back(std::move(transliteration));
        }
    }
    return true;
}

// An agent as the file remembers it, before it has a place in a document.
struct AgentRecord {
    Id id = InvalidId;
    AgentType type = AgentType::Other;
    QString name;
};

// One element of any kind, decoded into the payloads all three kinds share.
struct ElementRecord {
    Id id = InvalidId;
    ElementKind kind = ElementKind::Line;
    QString name;          ///< a section's name, an instrumental's description
    OptionalTiming timing; ///< for those two kinds
    Line line{InvalidId};  ///< the payload of a line
};

bool decodeAgent(const QJsonObject &object, AgentRecord *out)
{
    qint64 id = InvalidId;
    QString type;
    if (!readInt(object, QStringLiteral("id"), &id) || id < 1 ||
        !readString(object, QStringLiteral("type"), &type) ||
        !readString(object, QStringLiteral("name"), &out->name))
        return false;

    if (type == QLatin1String("person"))
        out->type = AgentType::Person;
    else if (type == QLatin1String("group"))
        out->type = AgentType::Group;
    else if (type == QLatin1String("other"))
        out->type = AgentType::Other;
    else
        return false;

    out->id = static_cast<Id>(id);
    return true;
}

bool decodeElement(const QJsonObject &object, ElementRecord *out)
{
    qint64 id = InvalidId;
    QString kind;
    if (!readInt(object, QStringLiteral("id"), &id) || id < 1 ||
        !readString(object, QStringLiteral("kind"), &kind))
        return false;
    out->id = static_cast<Id>(id);

    if (kind == QLatin1String("line")) {
        out->kind = ElementKind::Line;
        return readLine(object, &out->line);
    }
    if (kind == QLatin1String("section")) {
        out->kind = ElementKind::Section;
        return readString(object, QStringLiteral("name"), &out->name) &&
               readOptionalTiming(object, QStringLiteral("timing"), &out->timing);
    }
    if (kind == QLatin1String("instrumental")) {
        out->kind = ElementKind::Instrumental;
        return readString(object, QStringLiteral("description"), &out->name) &&
               readOptionalTiming(object, QStringLiteral("timing"), &out->timing);
    }
    return false;
}

} // namespace

QByteArray toJson(const Lyrics &document)
{
    QJsonObject root;
    root.insert(QStringLiteral("version"), JsonVersion);

    const Metadata &metadata = document.metadata();
    QJsonObject meta;
    meta.insert(QStringLiteral("title"), metadata.title);
    meta.insert(QStringLiteral("artist"), metadata.artist);
    meta.insert(QStringLiteral("album"), metadata.album);
    meta.insert(QStringLiteral("language"), metadata.language);
    root.insert(QStringLiteral("metadata"), meta);

    QJsonArray agents;
    for (const Agent &agent : document.agents()) {
        QJsonObject item;
        item.insert(QStringLiteral("id"), static_cast<qint64>(agent.id));
        item.insert(QStringLiteral("type"), agentTypeName(agent.type));
        item.insert(QStringLiteral("name"), agent.name);
        agents.append(item);
    }
    root.insert(QStringLiteral("agents"), agents);

    QJsonArray elements;
    for (const std::unique_ptr<Element> &element : document.elements())
        elements.append(elementJson(*element));
    root.insert(QStringLiteral("elements"), elements);

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

std::optional<Lyrics> fromJson(const QByteArray &json)
{
    QJsonParseError error;
    const QJsonDocument parsed = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !parsed.isObject())
        return std::nullopt;

    const QJsonObject root = parsed.object();
    qint64 version = 0;
    if (!readInt(root, QStringLiteral("version"), &version) || version != JsonVersion)
        return std::nullopt;

    const QJsonObject meta = root.value(QStringLiteral("metadata")).toObject();
    Metadata metadata;
    if (!readString(meta, QStringLiteral("title"), &metadata.title) ||
        !readString(meta, QStringLiteral("artist"), &metadata.artist) ||
        !readString(meta, QStringLiteral("album"), &metadata.album) ||
        !readString(meta, QStringLiteral("language"), &metadata.language))
        return std::nullopt;

    QJsonArray agentArray;
    QJsonArray elementArray;
    if (!readArray(root, QStringLiteral("agents"), &agentArray) ||
        !readArray(root, QStringLiteral("elements"), &elementArray))
        return std::nullopt;

    std::vector<AgentRecord> agents;
    agents.reserve(static_cast<std::size_t>(agentArray.size()));
    for (const QJsonValue &value : agentArray) {
        if (!value.isObject())
            return std::nullopt;
        AgentRecord record;
        if (!decodeAgent(value.toObject(), &record))
            return std::nullopt;
        agents.push_back(std::move(record));
    }

    std::vector<ElementRecord> elements;
    elements.reserve(static_cast<std::size_t>(elementArray.size()));
    for (const QJsonValue &value : elementArray) {
        if (!value.isObject())
            return std::nullopt;
        ElementRecord record;
        if (!decodeElement(value.toObject(), &record))
            return std::nullopt;
        elements.push_back(std::move(record));
    }

    Lyrics document;
    document.metadata() = std::move(metadata);

    // Agents and elements draw their ids from one counter, so they are replayed
    // in id order and each id is checked as it is handed out: a document that
    // does not reproduce its ids is refused, rather than left with a
    // translation pointing at the wrong line.
    std::size_t agentIndex = 0;
    std::size_t elementIndex = 0;
    while (agentIndex < agents.size() || elementIndex < elements.size()) {
        const bool takeAgent =
            elementIndex >= elements.size() ||
            (agentIndex < agents.size() && agents[agentIndex].id <= elements[elementIndex].id);

        if (takeAgent) {
            const AgentRecord &record = agents[agentIndex++];
            if (document.addAgent(record.type, record.name) != record.id)
                return std::nullopt;
            continue;
        }

        ElementRecord &record = elements[elementIndex++];
        if (record.kind == ElementKind::Line) {
            Line &line = document.addLine();
            if (line.id() != record.id)
                return std::nullopt;
            line.mainVocal = std::move(record.line.mainVocal);
            line.backgrounds = std::move(record.line.backgrounds);
            line.translations = std::move(record.line.translations);
            line.transliterations = std::move(record.line.transliterations);
        } else if (record.kind == ElementKind::Section) {
            Section &section = document.addSection(record.name);
            if (section.id() != record.id)
                return std::nullopt;
            section.timing = record.timing;
        } else {
            Instrumental &instrumental = document.addInstrumental(record.name);
            if (instrumental.id() != record.id)
                return std::nullopt;
            instrumental.timing = record.timing;
        }
    }

    return document;
}

} // namespace lyrics
