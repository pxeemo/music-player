// lyrics.h - the format-independent lyrics model.
//
// This is plain data: no QObject, no QML, no knowledge of any source format.
// Concrete parsers (LRC, TTML, embedded tags, ...) populate it and the Qt-facing
// LyricsDocument turns it into something the UI can bind to.
//
// Ownership and references
// ------------------------
//  * `Lyrics` owns its metadata, its agents (by value) and its elements
//    (`std::unique_ptr<Element>`), so element addresses stay stable while more
//    are appended.
//  * Cross-references (a vocal's agent, a translation's source line, a
//    transliteration's source) use stable `Id`s rather than raw pointers. Ids
//    survive copies, serialization and reordering, and can never dangle.
//  * Timing is optional at every level and is never invented: a source that only
//    gives a line's start leaves `end` empty, and a source with no timing at all
//    leaves the whole `OptionalTiming` empty.
//
// Element kinds are open for extension: a new kind needs an `ElementKind` value
// and an `Element` subclass. `Line`, `Section` and `Instrumental` are the
// initial set.

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <QChar>
#include <QString>

namespace lyrics {

/// Stable identifier for agents and elements. `InvalidId` means "none".
using Id = std::uint64_t;
inline constexpr Id InvalidId = 0;

/// A point in time, relative to the start of the track.
using Timestamp = std::chrono::milliseconds;

/// An optional time range. `end` is empty when the source only provided a
/// start; the end is then implied by the following element.
struct Timing {
    Timestamp start{};
    std::optional<Timestamp> end{};
};

/// Empty when the source provides no timing for this item at all.
using OptionalTiming = std::optional<Timing>;

enum class TextDirection {
    Ltr,
    Rtl,
};

/// First strong character wins; defaults to left-to-right.
TextDirection textDirection(const QString &text);

/// The smallest sung unit. Only present when the source times at this level.
struct Syllable {
    QString text;
    OptionalTiming timing;

    TextDirection direction() const;
};

/// A sung word, optionally split into syllables.
struct Word {
    QString text;
    OptionalTiming timing;
    std::vector<Syllable> syllables;

    TextDirection direction() const;
};

enum class AgentType {
    Person,
    Group,
    Other,
};

/// A singer or speaker. Agents are shared, so lines carry an `Id`, not a copy.
struct Agent {
    Id id = InvalidId;
    AgentType type = AgentType::Other;
    QString name;
};

/// A translation of the line it belongs to. Timing is inherited from the source
/// line, so it is deliberately not stored here - only the relationship is.
struct Translation {
    QString language; ///< e.g. "fa", "de", "ja"
    QString text;
    Id sourceLineId = InvalidId; ///< the line this translates
};

/// The original lyric rendered in another writing system. This is not a
/// translation: it represents the same words, not their meaning.
struct Transliteration {
    QString text;
    Id sourceId = InvalidId; ///< the lyric (line) it transliterates
};

/// A block of sung text: the main vocal of a line, or a background vocal.
/// Backgrounds own their own, independent timing and agent.
struct Vocal {
    QString text;
    OptionalTiming timing;
    std::vector<Word> words;
    Id agentId = InvalidId;

    TextDirection direction() const;
};

enum class ElementKind {
    Line,
    Section,
    Instrumental,
};

/// Base for anything that appears in the ordered element list.
class Element {
  public:
    virtual ~Element() = default;

    Id id() const { return m_id; }
    ElementKind kind() const { return m_kind; }

  protected:
    Element(Id id, ElementKind kind) : m_id(id), m_kind(kind) {}

  private:
    Id m_id;
    ElementKind m_kind;
};

/// The actual lyric content: a main vocal, plus any backgrounds, translations
/// and transliterations that belong to it.
class Line : public Element {
  public:
    explicit Line(Id id) : Element(id, ElementKind::Line) {}

    Vocal mainVocal;                ///< the line itself
    std::vector<Vocal> backgrounds; ///< backing vocals, independent timing
    std::vector<Translation> translations;
    std::vector<Transliteration> transliterations;

    const QString &text() const { return mainVocal.text; }
    const OptionalTiming &timing() const { return mainVocal.timing; }
};

/// A structural marker such as "Intro", "Verse 1", "Chorus", "Bridge", "Outro".
class Section : public Element {
  public:
    explicit Section(Id id) : Element(id, ElementKind::Section) {}

    QString name;
    OptionalTiming timing;
};

/// A timed stretch with no lyric text.
class Instrumental : public Element {
  public:
    explicit Instrumental(Id id) : Element(id, ElementKind::Instrumental) {}

    QString description; ///< optional label, e.g. "guitar solo"
    OptionalTiming timing;
};

struct Metadata {
    QString title;
    QString artist;
    QString album;
    QString language; ///< primary language of the lyrics, if known
};

/// The root of a lyrics document: metadata plus the ordered element list.
class Lyrics {
  public:
    Lyrics() = default;
    Lyrics(const Lyrics &) = delete;
    Lyrics &operator=(const Lyrics &) = delete;
    Lyrics(Lyrics &&) noexcept = default;
    Lyrics &operator=(Lyrics &&) noexcept = default;
    ~Lyrics() = default;

    Metadata &metadata() { return m_metadata; }
    const Metadata &metadata() const { return m_metadata; }

    /// Allocates a fresh id. The `add*` helpers below call this.
    Id newId() { return m_nextId++; }

    /// Registers an agent and returns its id.
    Id addAgent(AgentType type, QString name);
    const std::vector<Agent> &agents() const { return m_agents; }
    /// Returns null for `InvalidId` or an unknown id.
    const Agent *agent(Id id) const;

    /// Appends an element and returns a reference for the parser to fill in.
    /// The reference stays valid as later elements are appended.
    Line &addLine();
    Section &addSection(QString name = {});
    Instrumental &addInstrumental(QString description = {});

    const std::vector<std::unique_ptr<Element>> &elements() const { return m_elements; }
    std::size_t elementCount() const { return m_elements.size(); }
    /// Returns null for `InvalidId` or an unknown id.
    const Element *element(Id id) const;

  private:
    Metadata m_metadata;
    std::vector<Agent> m_agents;
    std::vector<std::unique_ptr<Element>> m_elements;
    Id m_nextId = 1;
};

} // namespace lyrics
