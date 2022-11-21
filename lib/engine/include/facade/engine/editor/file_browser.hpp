#pragma once
#include <facade/engine/editor/browse_file.hpp>

namespace facade::editor {
///
/// \brief Persistent modal pop up to browse for a file given a list of extensions.
///
/// Uses Modal and BrowseFile.
///
class FileBrowser {
  public:
	struct Result {
		std::string selected{};
		std::string_view browse_path{};
		bool path_changed{};
	};

	FileBrowser(std::string label, std::vector<std::string> extensions, std::string browse_path);

	Result update(bool& out_trigger);

	std::vector<std::string> extensions;

  private:
	std::string m_browse_path;
	std::string m_label;
	std::vector<std::string_view> m_extensions_view{};
	env::DirEntries m_dir_entries{};
	bool m_trigger{};
};
} // namespace facade::editor
