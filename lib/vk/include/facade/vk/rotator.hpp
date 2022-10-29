#pragma once
#include <cstdint>

namespace facade {
///
/// \brief Default buffering.
///
inline constexpr std::size_t buffering_v{2};

///
/// \brief Rotating array of Type.
///
template <typename Type, std::size_t Size = buffering_v>
struct Rotator {
	Type t[Size]{};
	std::size_t index{};

	constexpr Type& get() { return t[index]; }
	constexpr Type const& get() const { return t[index]; }

	constexpr void rotate() { index = (index + 1) % Size; }
};
} // namespace facade
