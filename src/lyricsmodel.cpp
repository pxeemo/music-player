#include "lyricsmodel.h"

#include <utility>

namespace lyrics {

Id Lyrics::addAgent(AgentType type, QString name)
{
	const Id id = newId();
	m_agents.push_back(Agent{id, type, std::move(name)});
	return id;
}

const Agent *Lyrics::agent(Id id) const
{
	if (id == InvalidId)
		return nullptr;
	for (const Agent &candidate : m_agents) {
		if (candidate.id == id)
			return &candidate;
	}
	return nullptr;
}

Line &Lyrics::addLine()
{
	auto element = std::make_unique<Line>(newId());
	Line *line = element.get();
	m_elements.push_back(std::move(element));
	return *line;
}

Section &Lyrics::addSection(QString name)
{
	auto element = std::make_unique<Section>(newId());
	element->name = std::move(name);
	Section *section = element.get();
	m_elements.push_back(std::move(element));
	return *section;
}

Instrumental &Lyrics::addInstrumental(QString description)
{
	auto element = std::make_unique<Instrumental>(newId());
	element->description = std::move(description);
	Instrumental *instrumental = element.get();
	m_elements.push_back(std::move(element));
	return *instrumental;
}

const Element *Lyrics::element(Id id) const
{
	if (id == InvalidId)
		return nullptr;
	for (const std::unique_ptr<Element> &candidate : m_elements) {
		if (candidate->id() == id)
			return candidate.get();
	}
	return nullptr;
}

}  // namespace lyrics
