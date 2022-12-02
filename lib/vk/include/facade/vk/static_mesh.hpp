#pragma once
#include <facade/vk/defer.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/gfx.hpp>

namespace facade {
class StaticMesh {
  public:
	struct Info {
		std::uint32_t vertices{};
		std::uint32_t indices{};
	};

	StaticMesh(Gfx const& gfx, Geometry::Packed const& geometry, std::string name = "(Unnamed)");

	std::string_view name() const { return m_name; }
	Info info() const;

	void draw(vk::CommandBuffer cb, std::size_t instances) const;

  private:
	BufferView vbo() const;
	BufferView ibo() const;

	Defer<UniqueBuffer> m_buffer{};
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
