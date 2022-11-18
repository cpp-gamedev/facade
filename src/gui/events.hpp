#pragma once
#include <string_view>

namespace facade::event {
struct Shutdown {};
struct OpenRecent {
	std::string_view path{};
};
struct OpenFile {};
struct BrowseCd {
	std::string_view path{};
};
struct FileDrop {
	std::string_view path{};
};
} // namespace facade::event
