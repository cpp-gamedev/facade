#include <facade/vk/cmd.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/static_mesh.hpp>
#include <span>

namespace facade {
namespace {
constexpr auto flags_v = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
} // namespace

StaticMesh::StaticMesh(Gfx const& gfx, Geometry const& geometry, std::string name) : m_name(std::move(name)), m_buffer{gfx.shared->defer_queue} {
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

MeshView StaticMesh::view() const { return {vbo(), ibo(), m_vertices, m_indices}; }

BufferView StaticMesh::vbo() const { return {m_buffer.get().get().buffer, m_vbo_size}; }

BufferView StaticMesh::ibo() const {
	if (m_vbo_size == m_buffer.get().get().size) { return {}; }
	return {m_buffer.get().get().buffer, m_buffer.get().get().size - m_vbo_size, m_vbo_size};
}
} // namespace facade
