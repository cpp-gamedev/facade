#pragma once
#include <facade/scene/id.hpp>
#include <facade/scene/interpolator.hpp>

namespace facade {
struct Node;

struct Animator {
	Interpolator<glm::vec3> translation{};
	Interpolator<glm::quat> rotation{};
	Interpolator<glm::vec3> scale{};

	float elapsed{};

	void update(Node& out_node, float dt);
};
} // namespace facade
