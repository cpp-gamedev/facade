#pragma once
#include <facade/engine/editor/common.hpp>
#include <facade/util/env.hpp>

namespace facade::editor {
struct BrowseFile {
	env::DirEntries& out_entries;
	char const* popup_name{"Browse..."};
	env::MatchList extensions{};

	std::string operator()(std::string& out_path);
};
} // namespace facade::editor
