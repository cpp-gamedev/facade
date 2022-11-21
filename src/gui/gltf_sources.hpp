#pragma once
#include <events/events.hpp>
#include <facade/engine/editor/file_browser.hpp>
#include <facade/util/env.hpp>
#include <gui/events.hpp>
#include <gui/path_source.hpp>

namespace facade {
class FileBrowser : public PathSource {
  public:
	FileBrowser(std::shared_ptr<Events> const& events, std::string browse_path);

  private:
	std::weak_ptr<Events> m_events;
	Observer<event::OpenFile> m_observer;
	editor::FileBrowser m_browser;
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
