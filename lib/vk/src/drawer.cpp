#include <facade/vk/drawer.hpp>

namespace facade {
Drawer::Drawer(Gfx const& gfx) : m_gfx{gfx}, m_mutex{std::make_unique<std::mutex>()} {}

void Drawer::draw(vk::CommandBuffer const cb, MeshView mesh, std::span<glm::mat4x4 const> instances) {
	if (!m_gfx.device) { return; }
	auto& instance_buffer = get_or_make();
	if (instances.size_bytes() > instance_buffer.get().size) {
		instance_buffer = m_gfx.vma.make_buffer(vk::BufferUsageFlagBits::eVertexBuffer, instances.size_bytes(), true);
	}
	std::memcpy(instance_buffer.get().ptr, instances.data(), instances.size_bytes());
	vk::Buffer const buffers[] = {mesh.vertices.buffer, instance_buffer.get().buffer};
	vk::DeviceSize const offsets[] = {0, 0};
	cb.bindVertexBuffers(0u, buffers, offsets);
	auto const count = static_cast<std::uint32_t>(instances.size());
	if (mesh.index_count > 0) {
		cb.bindIndexBuffer(mesh.indices.buffer, mesh.indices.offset, vk::IndexType::eUint32);
		cb.drawIndexed(mesh.index_count, count, 0u, 0u, 0u);
	} else {
		cb.draw(mesh.vertex_count, count, 0u, 0u);
	}
}

UniqueBuffer& Drawer::get_or_make() {
	auto lock = std::scoped_lock{*m_mutex};
	if (m_index < m_instance_buffers.size()) { return m_instance_buffers[m_index++]; }
	++m_index;
	return m_instance_buffers.emplace_back();
}
} // namespace facade
