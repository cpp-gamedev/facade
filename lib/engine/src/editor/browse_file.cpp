#include <imgui.h>
#include <facade/engine/editor/browse_file.hpp>
#include <facade/util/env.hpp>
#include <filesystem>

namespace facade::editor {
namespace fs = std::filesystem;

std::string BrowseFile::operator()(std::string& out_path) {
	if (auto popup = editor::Modal{popup_name}) {
		auto const fs_path = [&] {
			auto ret = fs::path{out_path};
			if (!fs::is_directory(ret)) {
				ret = fs::current_path();
				out_path = ret.generic_string();
			}
			return ret;
		}();
		ImGui::Text("%s", out_path.c_str());
		if (ImGui::Button("Up")) { out_path = fs_path.parent_path().generic_string(); }
		if (auto documents = env::documents_path(); !documents.empty()) {
			ImGui::SameLine();
			if (ImGui::Button("Documents")) { out_path = std::move(documents); }
		}
		if (auto downloads = env::downloads_path(); !downloads.empty()) {
			ImGui::SameLine();
			if (ImGui::Button("Downloads")) { out_path = std::move(downloads); }
		}

		ImGui::Separator();
		if (auto window = editor::Window{popup, "File Tree"}) {
			env::GetDirEntries{.extensions = extensions}(out_entries, out_path.c_str());
			if (fs_path.has_parent_path() && ImGui::Selectable("..", false, ImGuiSelectableFlags_DontClosePopups)) {
				out_path = fs_path.parent_path().generic_string();
			}
			for (auto const& dir : out_entries.dirs) {
				auto filename = env::to_filename(dir);
				filename += "/";
				if (ImGui::Selectable(filename.c_str(), false, ImGuiSelectableFlags_DontClosePopups)) { out_path = dir; }
			}
			for (auto const& file : out_entries.files) {
				if (ImGui::Selectable(env::to_filename(file).c_str(), false, ImGuiSelectableFlags_DontClosePopups)) {
					popup.close_current();
					return file;
				}
			}
		}
	}

	return {};
}
} // namespace facade::editor
