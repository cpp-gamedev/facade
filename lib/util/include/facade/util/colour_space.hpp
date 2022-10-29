#pragma once
#include <cstdint>

namespace facade {
///
/// \brief Represents a colour space: sRGB or linear
///
enum class ColourSpace : std::uint8_t { eSrgb, eLinear };
} // namespace facade
