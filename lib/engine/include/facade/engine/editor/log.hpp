#pragma once
#include <facade/engine/editor/common.hpp>
#include <facade/util/enum_array.hpp>
#include <facade/util/logger.hpp>
#include <vector>

namespace facade::editor {
struct LogState {
	EnumArray<logger::Level, bool> level_filter{true, true, true, debug_v};
	int display_count{50};
	bool auto_scroll{true};

	std::vector<logger::Entry const*> list{};
};

void display_log(NotClosed<Window> window, LogState& out_state);
} // namespace facade::editor
