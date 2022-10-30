#pragma once
#include <compare>

namespace facade {
///
/// \brief Semantic Version
///
struct Version {
	std::uint32_t major{};
	std::uint32_t minor{};
	std::uint32_t patch{};

	auto operator<=>(Version const&) const = default;
};
} // namespace facade
