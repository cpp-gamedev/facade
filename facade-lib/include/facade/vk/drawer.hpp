#pragma once
#include <facade/vk/gfx.hpp>
#include <glm/mat4x4.hpp>
#include <span>

namespace facade {
class Drawer {
  public:
	Drawer() = default;
	Drawer(Gfx const& gfx);

	void draw(vk::CommandBuffer cb, MeshView mesh, std::span<glm::mat4 const> instances);

  private:
	UniqueBuffer& get_or_make();

	std::vector<UniqueBuffer> m_instance_buffers{};
	Gfx m_gfx{};
	std::unique_ptr<std::mutex> m_mutex{};
	std::size_t m_index{};

	friend class Pipes;
};
} // namespace facade
