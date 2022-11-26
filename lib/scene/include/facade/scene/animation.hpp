#pragma once
#include <facade/scene/id.hpp>
#include <facade/scene/interpolator.hpp>

namespace facade {
struct Node;

struct TransformAnimator {
	Interpolator<glm::vec3> translation{};
	Interpolator<glm::quat> rotation{};
	Interpolator<glm::vec3> scale{};
	std::optional<Id<Node>> target{};

	float elapsed{};

	void update(Node& out_node, float dt);
};

struct Animation {
	TransformAnimator transform{};
};
} // namespace facade
