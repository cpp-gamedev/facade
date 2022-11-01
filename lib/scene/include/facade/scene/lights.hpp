#pragma once
#include <facade/util/flex_array.hpp>
#include <facade/util/nvec3.hpp>
#include <facade/util/rgb.hpp>

namespace facade {
///
/// \brief Directional light.
///
struct DirLight {
	///
	/// \brief Direction.
	///
	nvec3 direction{-front_v};
	///
	/// \brief Colour and intensity.
	///
	Rgb rgb{};
};

struct Lights {
	static constexpr std::size_t max_lights_v{4};

	FlexArray<DirLight, max_lights_v> dir_lights{};
};
} // namespace facade
