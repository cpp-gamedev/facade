#include <facade/vk/cmd.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/static_mesh.hpp>

namespace facade {
namespace {
constexpr auto flags_v = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
} // namespace

StaticMesh::StaticMesh(Gfx const& gfx, Geometry const& geometry, std::string name) : m_buffer(gfx.shared->defer_queue), m_name(std::move(name)) {
	auto const vertices = std::span<Vertex const>{geometry.vertices};
	auto const indices = std::span<std::uint32_t const>{geometry.indices};
	m_vertices = static_cast<std::uint32_t>(vertices.size());
	m_indices = static_cast<std::uint32_t>(indices.size());
	m_vbo_size = vertices.size_bytes();
	auto const size = m_vbo_size + indices.size_bytes();
	m_buffer.swap(gfx.vma.make_buffer(flags_v, size, false));
	auto staging = gfx.vma.make_buffer(vk::BufferUsageFlagBits::eTransferSrc, size, true);
	std::memcpy(staging.get().ptr, vertices.data(), vertices.size_bytes());
	if (!indices.empty()) { std::memcpy(static_cast<std::byte*>(staging.get().ptr) + m_vbo_size, indices.data(), indices.size_bytes()); }
	auto const bc = vk::BufferCopy{{}, {}, size};
	auto cmd = Cmd{gfx, vk::PipelineStageFlagBits::eTopOfPipe};
	cmd.cb.copyBuffer(staging.get().buffer, m_buffer.get().get().buffer, bc);
}

auto StaticMesh::info() const -> Info { return {m_vertices, m_indices}; }

void StaticMesh::draw(vk::CommandBuffer cb, std::size_t const instances, std::uint32_t binding) const {
	auto const count = static_cast<std::uint32_t>(instances);
	auto const& v = vbo();
	cb.bindVertexBuffers(binding, v.buffer, vk::DeviceSize{0});
	if (m_indices > 0) {
		auto const& i = ibo();
		cb.bindIndexBuffer(i.buffer, i.offset, vk::IndexType::eUint32);
		cb.drawIndexed(m_indices, count, 0u, 0u, 0u);
	} else {
		cb.draw(m_vertices, count, 0u, 0u);
	}
}

BufferView StaticMesh::vbo() const { return {.buffer = m_buffer.get().get().buffer, .size = m_vbo_size, .count = m_vertices}; }

BufferView StaticMesh::ibo() const {
	if (m_vbo_size == m_buffer.get().get().size) { return {}; }
	auto const& buffer = m_buffer.get().get();
	return {.buffer = buffer.buffer, .size = buffer.size - m_vbo_size, .offset = m_vbo_size, .count = m_indices};
}
} // namespace facade
