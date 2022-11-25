#pragma once
#include <facade/scene/animator.hpp>
#include <facade/scene/id.hpp>

namespace facade {
struct Animation {
	Animator animator{};
	Id<Node> target{};
};
} // namespace facade
