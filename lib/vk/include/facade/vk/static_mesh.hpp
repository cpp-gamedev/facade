#pragma once
#include <facade/vk/defer.hpp>
#include <facade/vk/gfx.hpp>

namespace facade {
struct Geometry;

class StaticMesh {
  public:
	StaticMesh(Gfx const& gfx, Geometry const& geometry, std::string name = "(Unnamed)");

	std::string_view name() const { return m_name; }
	MeshView view() const;

	void draw(vk::CommandBuffer cb, BufferView instances) const;

  private:
	BufferView vbo() const;
	BufferView ibo() const;

	std::string m_name{};
	Defer<UniqueBuffer> m_buffer{};
	std::size_t m_vbo_size{};
	std::uint32_t m_vertices{};
	std::uint32_t m_indices{};
};
} // namespace facade
