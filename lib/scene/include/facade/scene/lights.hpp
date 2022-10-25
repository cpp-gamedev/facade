#pragma once
#include <facade/util/nvec3.hpp>

namespace facade {
struct DirLight {
	alignas(16) glm::vec3 direction{front_v};
	alignas(16) glm::vec3 ambient{0.04f};
	alignas(16) glm::vec3 diffuse{1.0f};
};
} // namespace facade
