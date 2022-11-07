#pragma once
#include <string>

namespace facade::event {
struct OpenRecent {
	std::string path{};
};
struct OpenFile {};
struct BrowseCd {
	std::string path{};
};
struct FileDrop {
	std::string path{};
};
} // namespace facade::event
