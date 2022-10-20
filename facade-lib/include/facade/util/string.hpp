#pragma once
#include <sstream>

namespace facade {
template <typename... T>
std::string concat(T const&... t) {
	auto ret = std::stringstream{};
	((ret << t), ...);
	return ret.str();
}
} // namespace facade
