#pragma once
#include <facade/vk/defer.hpp>
#include <facade/vk/gfx.hpp>
#include <span>

namespace facade {
struct Geometry;

class StaticMesh {
  public:
	StaticMesh(Gfx const& gfx, Geometry const& geometry);

	MeshView view() const;
	operator MeshView() const { return view(); }

  private:
	BufferView vbo() const;
	BufferView ibo() const;

	Defer<UniqueBuffer> m_buffer{};
	std::size_t m_vbo_size{};
	std::uint32_t m_vertices{};
	std::uint32_t m_indices{};
};
} // namespace facade
