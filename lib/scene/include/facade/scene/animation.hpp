#pragma once
#include <facade/scene/id.hpp>
#include <facade/scene/interpolator.hpp>

namespace facade {
struct Node;

struct Animation {
	Interpolator<Transform> transform{};
	Id<Node> target{};
};
} // namespace facade
