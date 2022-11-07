#pragma once
#include <events/events.hpp>
#include <facade/util/env.hpp>
#include <gui/events.hpp>
#include <gui/path_source.hpp>

namespace facade {
struct BrowseGltf : PathSource {
	std::shared_ptr<Events> events;
	Observer<event::OpenFile> observer;
	bool trigger{};

	env::DirEntries dir_entries{};
	std::string browse_path{};

	BrowseGltf(std::shared_ptr<Events> events, std::string browse_path)
		: events(std::move(events)), observer(this->events, [this](event::OpenFile) { trigger = true; }), browse_path(std::move(browse_path)) {}

	std::string update() final;
};

struct OpenRecent : PathSource {
	Observer<event::OpenRecent> observer;
	std::string path{};

	OpenRecent(std::shared_ptr<Events> const& events) : observer(events, [this](event::OpenRecent const& recent) { this->path = recent.path; }) {}

	std::string update() final;
};

struct DropFile : PathSource {
	Observer<event::FileDrop> observer;
	std::string path{};

	DropFile(std::shared_ptr<Events> const& events) : observer(events, [this](event::FileDrop const& fd) { path = fd.path; }) {}

	std::string update() final;
};
} // namespace facade
