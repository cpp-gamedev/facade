#pragma once
#include <events/events.hpp>
#include <facade/util/env.hpp>
#include <gui/events.hpp>
#include <gui/path_source.hpp>

namespace facade {
class BrowseGltf : public PathSource {
  public:
	BrowseGltf(std::shared_ptr<Events> const& events, std::string browse_path);

  private:
	std::weak_ptr<Events> m_events;
	Observer<event::OpenFile> m_observer;
	env::DirEntries m_dir_entries{};
	std::string m_browse_path{};
	bool m_trigger{};

	std::string update() final;
};

class OpenRecent : public PathSource {
  public:
	OpenRecent(std::shared_ptr<Events> const& events);

  private:
	Observer<event::OpenRecent> m_observer;
	std::string m_path{};

	std::string update() final;
};

class DropFile : public PathSource {
  public:
	DropFile(std::shared_ptr<Events> const& events);

  private:
	Observer<event::FileDrop> m_observer;
	std::string m_path{};

	std::string update() final;
};
} // namespace facade
