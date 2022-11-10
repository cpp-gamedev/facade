#pragma once
#include <facade/engine/editor/common.hpp>
#include <facade/util/env.hpp>

namespace facade::editor {
struct BrowseFile {
	struct Result {
		std::string selected{};
		bool dir_changed{};
	};

	env::DirEntries& out_entries;
	env::MatchList extensions{};
	float parent_indent{5.0f};

	Result operator()(NotClosed<Popup> popup, std::string& out_path);
};
} // namespace facade::editor
