#pragma once
#include <facade/vk/defer.hpp>
#include <facade/vk/gfx.hpp>
#include <glm/vec4.hpp>
#include <span>

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

	SkinnedMesh(Gfx const& gfx, Geometry const& geometry, CreateInfo const& create_info, std::string name = "(Unnamed)");

	std::string_view name() const { return m_name; }
	Info info() const;

	void draw(vk::CommandBuffer cb) const;

  private:
	BufferView vbo() const;
	BufferView ibo() const;

	Defer<UniqueBuffer> m_vibo{};
	Defer<UniqueBuffer> m_joints{};
	Defer<UniqueBuffer> m_weights{};
	std::string m_name{};
	std::size_t m_vbo_size{};
	std::uint32_t m_vertices{};
	std::uint32_t m_indices{};

	struct Offsets {
		std::size_t positions{};
		std::size_t rgbs{};
		std::size_t normals{};
		std::size_t uvs{};
	};

	Offsets m_offsets{};
};
} // namespace facade
