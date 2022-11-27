#pragma once
#include <facade/vk/defer.hpp>
#include <facade/vk/gfx.hpp>

namespace facade {
struct Geometry;

class StaticMesh {
  public:
	struct Info {
		std::uint32_t vertices{};
		std::uint32_t indices{};
	};

	StaticMesh(Gfx const& gfx, Geometry const& geometry, std::string name = "(Unnamed)");

	std::string_view name() const { return m_name; }
	Info info() const;

	void draw(vk::CommandBuffer cb, std::size_t instances, std::uint32_t binding = 0u) const;

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
