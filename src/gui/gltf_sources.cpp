#include <imgui.h>
#include <facade/engine/editor/browse_file.hpp>
#include <facade/engine/editor/common.hpp>
#include <facade/util/logger.hpp>
#include <gui/gltf_sources.hpp>
#include <filesystem>
#include <utility>

namespace facade {
namespace {
namespace fs = std::filesystem;

fs::path find_file_with_ext(fs::path root, env::MatchList exts) {
	auto const match = [exts](std::string_view ext) { return std::any_of(exts.begin(), exts.end(), [ext](std::string_view rhs) { return ext == rhs; }); };
	if (match(root.extension().string())) { return root; }
	if (!fs::is_directory(root)) { return {}; }
	for (auto const& it : fs::directory_iterator{root}) {
		if (!it.is_regular_file()) { continue; }
		auto path = it.path();
		if (match(path.extension().string())) { return path; }
	}
	return {};
}
} // namespace

FileBrowser::FileBrowser(std::shared_ptr<Events> const& events, std::string browse_path)
	: m_events(events), m_observer(events, [this](event::OpenFile) { m_trigger = true; }),
	  m_browser("Browse...", {".gltf", ".skybox"}, std::move(browse_path)) {}

std::string FileBrowser::update() {
	auto result = m_browser.update(m_trigger);
	if (result.path_changed) {
		if (auto events = m_events.lock()) { events->dispatch(event::BrowseCd{result.browse_path}); }
	}
	return std::move(result.selected);
}

OpenRecent::OpenRecent(std::shared_ptr<Events> const& events) : m_observer(events, [this](event::OpenRecent const& recent) { m_path = recent.path; }) {}

std::string OpenRecent::update() { return std::exchange(m_path, {}); }

DropFile::DropFile(std::shared_ptr<Events> const& events) : m_observer(events, [this](event::FileDrop const& fd) { m_path = fd.path; }) {}

std::string DropFile::update() {
	if (m_path.empty()) { return {}; }
	static constexpr std::string_view exts_v[] = {".gltf", ".skybox"};
	if (auto ret = find_file_with_ext(m_path, exts_v); fs::is_regular_file(ret)) {
		m_path.clear();
		return ret.generic_string();
	}
	logger::error("Failed to locate .gltf / .skybox in path: [{}]", m_path);
	m_path.clear();
	return {};
}
} // namespace facade
