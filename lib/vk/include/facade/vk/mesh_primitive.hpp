#pragma once
#include <facade/vk/defer.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/vertex_layout.hpp>
#include <facade/vk/gfx.hpp>
#include <glm/vec4.hpp>

namespace facade {
class MeshPrimitive {
  public:
	class Builder;

	struct Info {
		std::uint32_t vertices{};
		std::uint32_t indices{};
	};

	std::string_view name() const { return m_name; }
	Info info() const;
	VertexLayout const& vertex_layout() const { return m_vlayout; }
	bool has_joints() const { return m_jwbo.get().get().size > 0; }

	void draw(vk::CommandBuffer cb, std::uint32_t instances = 1u) const;

  private:
	MeshPrimitive(Gfx const& gfx, std::string name);

	struct Offsets {
		std::size_t positions{};
		std::size_t rgbs{};
		std::size_t normals{};
		std::size_t uvs{};
		std::size_t joints{};
		std::size_t weights{};
		std::size_t indices{};
	};

	VertexLayout m_vlayout{};
	Defer<UniqueBuffer> m_vibo{};
	Defer<UniqueBuffer> m_jwbo{};
	Offsets m_offsets{};
	std::string m_name{};
	std::uint32_t m_vertices{};
	std::uint32_t m_indices{};
};

class MeshPrimitive::Builder {
  public:
	Builder(Gfx const& gfx, std::string name);

	MeshPrimitive operator()(Geometry::Packed const& geometry);
	MeshPrimitive operator()(Geometry::Packed const& geometry, std::span<glm::uvec4 const> joints, std::span<glm::vec4 const> weights);

  private:
	[[nodiscard]] UniqueBuffer upload(vk::CommandBuffer cb, Geometry::Packed const& geometry);
	[[nodiscard]] UniqueBuffer upload(vk::CommandBuffer cb, std::span<glm::uvec4 const> joints, std::span<glm::vec4 const> weights);

	Gfx m_gfx;
	MeshPrimitive m_ret;
	std::string m_name{"(Unnamed)"};
};
} // namespace facade
