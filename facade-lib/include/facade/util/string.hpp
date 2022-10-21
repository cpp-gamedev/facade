#pragma once
#include <sstream>

namespace facade {
template <typename... T>
std::stringstream& concat_to(std::stringstream& out, T const&... t) {
	((out << t), ...);
	return out;
}

template <typename... T>
std::string concat(T const&... t) {
	auto ret = std::stringstream{};
	concat_to(ret, t...);
	return ret.str();
}
} // namespace facade
