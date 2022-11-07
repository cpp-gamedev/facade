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

std::string BrowseGltf::update() {
	if (trigger) {
		editor::Popup::open("Browse...");
		trigger = false;
	}
	if (ImGui::IsPopupOpen("Browse...")) { ImGui::SetNextWindowSize({400.0f, 250.0f}, ImGuiCond_FirstUseEver); }
	if (auto popup = editor::Modal{"Browse..."}) {
		static constexpr std::string_view gltf_ext_v[] = {".gltf"};
		auto [selected, dir_changed] = editor::BrowseFile{.out_entries = dir_entries, .extensions = gltf_ext_v}(popup, browse_path);
		if (dir_changed) { events->dispatch(event::BrowseCd{browse_path}); }
		if (!selected.empty()) {
			popup.close_current();
			return selected;
		}
	}
	return {};
}

std::string OpenRecent::update() { return std::exchange(path, {}); }

std::string DropFile::update() {
	if (path.empty()) { return {}; }
	if (auto ret = find_gltf(path); fs::is_regular_file(ret)) {
		path.clear();
		return ret.generic_string();
	}
	logger::error("Failed to locate .gltf in path: [{}]", path);
	path.clear();
	return {};
}
} // namespace facade
