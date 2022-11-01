#pragma once
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
	nvec3 direction{front_v};
	///
	/// \brief Colour and intensity.
	///
	Rgb rgb{};
};
} // namespace facade
