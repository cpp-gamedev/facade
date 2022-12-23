#pragma once
#include <facade/vk/defer.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/gfx.hpp>
#include <facade/vk/vertex_layout.hpp>
#include <glm/vec4.hpp>
#include <optional>

namespace facade {
struct MeshJoints {
	std::span<glm::uvec4 const> joints{};
	std::span<glm::vec4 const> weights{};
};

class MeshPrimitive {
  public:
	struct Info {
		std::uint32_t vertices{};
		std::uint32_t indices{};
	};

	using Joints = MeshJoints;

	MeshPrimitive(Gfx const& gfx, Geometry::Packed const& geometry, Joints joints = {}, std::string name = "(Unnamed)");
	MeshPrimitive(Gfx const& gfx, Geometry const& geometry, Joints joints = {}, std::string name = "(Unnamed)");

	std::string_view name() const { return m_name; }
	Info info() const;
	VertexLayout const& vertex_layout() const { return m_vlayout; }
	bool has_joints() const { return m_jwbo.get().get().size > 0; }
	std::optional<std::uint32_t> joints_set() const { return m_jwbo.get().get().size > 0 ? std::optional<std::uint32_t>{3} : std::nullopt; }
	std::uint32_t instance_binding() const { return m_instance_binding; }

	void draw(vk::CommandBuffer cb, std::uint32_t instances = 1u) const;

  private:
	struct Uploader;
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
	std::uint32_t m_instance_binding{};
};
} // namespace facade
