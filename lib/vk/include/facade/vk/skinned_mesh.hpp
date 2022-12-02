#pragma once
#include <facade/vk/defer.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/gfx.hpp>
#include <glm/vec4.hpp>

namespace facade {
struct Geometry;

class SkinnedMesh {
  public:
	struct CreateInfo {
		std::span<glm::uvec4 const> joints{};
		std::span<glm::vec4 const> weights{};
	};

	struct Info {
		std::uint32_t vertices{};
		std::uint32_t indices{};
	};

	SkinnedMesh(Gfx const& gfx, Geometry::Packed const& geometry, CreateInfo const& create_info, std::string name = "(Unnamed)");

	std::string_view name() const { return m_name; }
	Info info() const;
	bool has_joints() const { return m_jwbo.get().get().size > 0; }

	void draw(vk::CommandBuffer cb, std::uint32_t instances = 1u) const;

  private:
	Defer<UniqueBuffer> m_vibo{};
	Defer<UniqueBuffer> m_jwbo{};
	std::string m_name{};
	std::uint32_t m_vertices{};
	std::uint32_t m_indices{};

	struct Offsets {
		std::size_t positions{};
		std::size_t rgbs{};
		std::size_t normals{};
		std::size_t uvs{};
		std::size_t joints{};
		std::size_t weights{};
		std::size_t indices{};
	};

	Offsets m_offsets{};
};
} // namespace facade
