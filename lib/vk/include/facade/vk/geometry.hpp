#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <span>
#include <vector>

namespace facade {
struct Vertex {
	glm::vec3 position{};
	glm::vec3 rgb{1.0f};
	glm::vec3 normal{0.0f, 0.0f, 1.0f};
	glm::vec2 uv{};
};

struct Geometry {
	std::vector<glm::vec3> positions{};
	std::vector<glm::vec3> rgbs{};
	std::vector<glm::vec3> normals{};
	std::vector<glm::vec2> uvs{};
	std::vector<std::uint32_t> indices{};

	Geometry& reserve(std::size_t vertices, std::size_t indices);

	Geometry& append(Geometry const& geometry);
	Geometry& append(std::span<Vertex const> vs, std::span<std::uint32_t const> is);
	Geometry& append_cube(glm::vec3 size, glm::vec3 rgb = glm::vec3{1.0f}, glm::vec3 origin = {});
};

Geometry make_cube(glm::vec3 size, glm::vec3 rgb = glm::vec3{1.0f}, glm::vec3 origin = {});
Geometry make_cubed_sphere(float diam, std::uint32_t quads_per_side, glm::vec3 rgb = glm::vec3{1.0f});
Geometry make_cone(float xz_diam, float y_height, std::uint32_t xz_points, glm::vec3 rgb = glm::vec3{1.0f});
Geometry make_cylinder(float xz_diam, float y_height, std::uint32_t xz_points, glm::vec3 rgb = glm::vec3{1.0f});
Geometry make_arrow(float stalk_diam, float stalk_height, std::uint32_t xz_points, glm::vec3 rgb = glm::vec3{1.0f});
Geometry make_manipulator(float stalk_diam, float stalk_height, std::uint32_t xz_points, glm::vec3 rgb = glm::vec3{1.0f});
} // namespace facade
