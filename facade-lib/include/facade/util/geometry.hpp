#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace facade {
struct Vertex {
	glm::vec3 position{};
	glm::vec3 rgb{1.0f};
	glm::vec3 normal{0.0f, 0.0f, 1.0f};
	glm::vec2 uv{};
};

struct Geometry {
	std::vector<Vertex> vertices{};
	std::vector<std::uint32_t> indices{};
};

Geometry make_cube(glm::vec3 size, glm::vec3 rgb = glm::vec3{1.0f}, glm::vec3 origin = {});
Geometry make_cubed_sphere(float diam, std::uint32_t quads_per_side, glm::vec3 rgb = glm::vec3{1.0f});
} // namespace facade
