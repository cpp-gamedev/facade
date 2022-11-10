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

fs::path find_gltf(fs::path root) {
	if (root.extension() == ".gltf") { return root; }
	if (!fs::is_directory(root)) { return {}; }
	for (auto const& it : fs::directory_iterator{root}) {
		if (!it.is_regular_file()) { continue; }
		auto path = it.path();
		if (path.extension() == ".gltf") { return path; }
	}
	return {};
}
} // namespace

BrowseGltf::BrowseGltf(std::shared_ptr<Events> const& events, std::string browse_path)
	: m_events(events), m_observer(events, [this](event::OpenFile) { m_trigger = true; }), m_browse_path(std::move(browse_path)) {}

std::string BrowseGltf::update() {
	if (m_trigger) {
		editor::Popup::open("Browse...");
		m_trigger = false;
	}
	if (ImGui::IsPopupOpen("Browse...")) { ImGui::SetNextWindowSize({400.0f, 250.0f}, ImGuiCond_FirstUseEver); }
	if (auto popup = editor::Modal{"Browse..."}) {
		static constexpr std::string_view gltf_ext_v[] = {".gltf"};
		auto [selected, dir_changed] = editor::BrowseFile{.out_entries = m_dir_entries, .extensions = gltf_ext_v}(popup, m_browse_path);
		if (dir_changed) {
			if (auto events = m_events.lock()) { events->dispatch(event::BrowseCd{m_browse_path}); }
		}
		if (!selected.empty()) {
			popup.close_current();
			return selected;
		}
	}
	return {};
}

OpenRecent::OpenRecent(std::shared_ptr<Events> const& events) : m_observer(events, [this](event::OpenRecent const& recent) { m_path = recent.path; }) {}

std::string OpenRecent::update() { return std::exchange(m_path, {}); }

DropFile::DropFile(std::shared_ptr<Events> const& events) : m_observer(events, [this](event::FileDrop const& fd) { m_path = fd.path; }) {}

std::string DropFile::update() {
	if (m_path.empty()) { return {}; }
	if (auto ret = find_gltf(m_path); fs::is_regular_file(ret)) {
		m_path.clear();
		return ret.generic_string();
	}
	logger::error("Failed to locate .gltf in path: [{}]", m_path);
	m_path.clear();
	return {};
}
} // namespace facade
