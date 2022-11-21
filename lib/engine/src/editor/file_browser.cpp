#include <imgui.h>
#include <facade/engine/editor/file_browser.hpp>

namespace facade::editor {
FileBrowser::FileBrowser(std::string label, std::vector<std::string> extensions, std::string browse_path)
	: extensions(std::move(extensions)), m_browse_path(std::move(browse_path)), m_label(std::move(label)) {
	if (m_label.empty()) { m_label = "Browse..."; }
}

auto FileBrowser::update(bool& out_trigger) -> Result {
	auto ret = Result{};
	if (out_trigger && !ImGui::IsPopupOpen(m_label.c_str())) { Modal::open(m_label.c_str()); }
	out_trigger = false;
	if (ImGui::IsPopupOpen(m_label.c_str())) { ImGui::SetNextWindowSize({400.0f, 250.0f}, ImGuiCond_FirstUseEver); }
	if (auto popup = Modal{m_label.c_str()}) {
		m_extensions_view.clear();
		m_extensions_view.reserve(extensions.size());
		for (auto const& extension : extensions) { m_extensions_view.push_back(extension); }
		auto [selected, changed] = BrowseFile{.out_entries = m_dir_entries, .extensions = m_extensions_view}(popup, m_browse_path);
		ret.path_changed = changed;
		ret.browse_path = m_browse_path;
		if (!selected.empty()) {
			popup.close_current();
			ret.selected = std::move(selected);
		}
	}
	return ret;
}
} // namespace facade::editor
