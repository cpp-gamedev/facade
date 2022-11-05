#pragma once
#include <facade/engine/editor/common.hpp>
#include <facade/util/env.hpp>

namespace facade::editor {
struct BrowseFile {
	env::DirEntries& out_entries;
	env::MatchList extensions{};
	float parent_indent{5.0f};

	std::string operator()(NotClosed<Popup> popup, std::string& out_path);
};
} // namespace facade::editor
