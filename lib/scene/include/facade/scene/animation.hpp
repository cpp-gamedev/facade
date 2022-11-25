#pragma once
#include <facade/scene/animator.hpp>

namespace facade {
struct Animation {
	Animator animator{};
	Id<Node> target{};
};
} // namespace facade
