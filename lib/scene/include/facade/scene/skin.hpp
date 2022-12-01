#pragma once
#include <facade/scene/id.hpp>
#include <glm/mat4x4.hpp>
#include <optional>

namespace facade {
struct Node;

struct Skin {
	std::vector<glm::mat4x4> inverse_bind_matrices{};
	std::optional<Id<Node>> skeleton{};
	std::vector<Id<Node>> joints{};
	std::string name{};
};
} // namespace facade
