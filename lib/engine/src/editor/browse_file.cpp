#include <fmt/format.h>
#include <imgui.h>
#include <facade/engine/editor/browse_file.hpp>
#include <facade/util/env.hpp>
#include <facade/util/fixed_string.hpp>
#include <filesystem>

namespace facade::editor {
namespace fs = std::filesystem;

namespace {
constexpr bool drop_leading_dir(std::string_view& out) {
	if (!out.empty() && out[0] == '/') { out = out.substr(1); }
	if (auto const i = out.find_first_of('/'); i != std::string_view::npos) {
		out = out.substr(i + 1);
		return true;
	}
	return false;
}

std::string truncate(std::string_view in, std::size_t limit) {
	if (in.size() <= limit) { return std::string{in}; }
	while (in.size() + 4 > limit && drop_leading_dir(in))
		;
	return fmt::format(".../{}", in);
}
} // namespace

auto BrowseFile::operator()(NotClosed<Popup> popup, std::string& out_path) -> Result {
	auto ret = Result{};
	auto fs_path = [&] {
		auto path = fs::path{};
		if (!fs::is_directory(out_path)) {
			path = fs::current_path();
			out_path = path.generic_string();
			ret.dir_changed = true;
		} else {
			path = fs::absolute(out_path);
		}
		return path;
	}();
	auto cd = [&](fs::path path) {
		fs_path = std::move(path);
		out_path = fs_path.generic_string();
		ret.dir_changed = true;
	};
	auto label = out_path;
	auto const text_size = ImGui::CalcTextSize(label.c_str());
	ImGui::PushItemWidth(-50.0f);
	if (auto const ratio = (ImGui::GetWindowWidth() - 100.0f) / text_size.x; ratio > 0.0f) {
		auto const limit = static_cast<std::size_t>(ratio * static_cast<float>(label.size()));
		label = truncate(label, limit);
	}
	if (ImGui::BeginCombo("Path", label.c_str())) {
		auto item_width = 0.0f;
		for (auto p = fs_path; !p.empty(); p = p.parent_path()) {
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + item_width);
			item_width += parent_indent;
			auto name = p.filename().generic_string();
			name += "/";
			if (ImGui::Selectable(name.c_str(), false)) {
				cd(std::move(p));
				break;
			}
			if (p.parent_path() == p) { break; }
		}
		ImGui::EndCombo();
	}
	if (fs_path.has_parent_path() && ImGui::Button("Up")) { cd(fs_path.parent_path()); }
	if (auto documents = env::documents_path(); !documents.empty()) {
		ImGui::SameLine();
		if (ImGui::Button("Documents")) { cd(std::move(documents)); }
	}
	if (auto downloads = env::downloads_path(); !downloads.empty()) {
		ImGui::SameLine();
		if (ImGui::Button("Downloads")) { cd(std::move(downloads)); }
	}

	ImGui::Separator();
	auto exts = FixedString{};
	if (extensions.empty()) {
		exts = "*.*";
	} else {
		exts = FixedString{"*{}", extensions[0]};
		for (auto it = std::next(extensions.begin()); it != extensions.end(); ++it) { exts += FixedString{", *{}", *it}; }
	}
	ImGui::Text("%s", exts.c_str());

	ImGui::Separator();
	if (auto window = editor::Window{popup, "File Tree"}) {
		env::GetDirEntries{.extensions = extensions}(out_entries, out_path.c_str());
		if (fs_path.has_parent_path() && ImGui::Selectable("..", false, ImGuiSelectableFlags_DontClosePopups)) { cd(fs_path.parent_path()); }
		for (auto const& dir : out_entries.dirs) {
			auto filename = env::to_filename(dir);
			filename += "/";
			if (ImGui::Selectable(filename.c_str(), false, ImGuiSelectableFlags_DontClosePopups)) { cd(dir); }
		}
		for (auto const& file : out_entries.files) {
			if (ImGui::Selectable(env::to_filename(file).c_str(), false, ImGuiSelectableFlags_DontClosePopups)) { ret.selected = file; }
		}
	}

	return ret;
}
} // namespace facade::editor
