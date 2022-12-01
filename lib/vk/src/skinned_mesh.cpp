#include <facade/util/error.hpp>
#include <facade/vk/cmd.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/skinned_mesh.hpp>

namespace facade {
namespace {
constexpr auto v_flags_v = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
constexpr auto vi_flags_v = v_flags_v | vk::BufferUsageFlagBits::eIndexBuffer;
} // namespace

SkinnedMesh::SkinnedMesh(Gfx const& gfx, Geometry const& geometry, CreateInfo const& create_info, std::string name)
	: m_vibo(gfx.shared->defer_queue), m_joints(gfx.shared->defer_queue), m_weights(gfx.shared->defer_queue), m_name(std::move(name)) {
	if (create_info.joints.size() != create_info.weights.size()) { throw Error{"Mismatched joints and weights"}; }

	auto const vertices = std::span<Vertex const>{geometry.vertices};
	auto const indices = std::span<std::uint32_t const>{geometry.indices};
	m_vertices = static_cast<std::uint32_t>(vertices.size());
	m_indices = static_cast<std::uint32_t>(indices.size());
	m_vbo_size = vertices.size_bytes();
	auto const size = m_vbo_size + indices.size_bytes();
	m_vibo.swap(gfx.vma.make_buffer(vi_flags_v, size, false));
	auto staging = gfx.vma.make_buffer(vk::BufferUsageFlagBits::eTransferSrc, size, true);
	std::memcpy(staging.get().ptr, vertices.data(), vertices.size_bytes());
	if (!indices.empty()) { std::memcpy(static_cast<std::byte*>(staging.get().ptr) + m_vbo_size, indices.data(), indices.size_bytes()); }
	auto cmd = Cmd{gfx, vk::PipelineStageFlagBits::eTopOfPipe};
	cmd.cb.copyBuffer(staging.get().buffer, m_vibo.get().get().buffer, vk::BufferCopy{{}, {}, size});

	if (!create_info.joints.empty()) {
		m_joints.swap(gfx.vma.make_buffer(v_flags_v, create_info.joints.size_bytes(), false));
		m_weights.swap(gfx.vma.make_buffer(v_flags_v, create_info.weights.size_bytes(), false));
		auto joints = gfx.vma.make_buffer(vk::BufferUsageFlagBits::eTransferSrc, m_joints.get().get().size, true);
		auto weights = gfx.vma.make_buffer(vk::BufferUsageFlagBits::eTransferSrc, m_weights.get().get().size, true);
		std::memcpy(joints.get().ptr, create_info.joints.data(), joints.get().size);
		std::memcpy(weights.get().ptr, create_info.weights.data(), weights.get().size);
		cmd.cb.copyBuffer(joints.get().buffer, m_joints.get().get().buffer, vk::BufferCopy{{}, {}, joints.get().size});
		cmd.cb.copyBuffer(weights.get().buffer, m_weights.get().get().buffer, vk::BufferCopy{{}, {}, weights.get().size});
	}
}

auto SkinnedMesh::info() const -> Info { return {m_vertices, m_indices}; }

void SkinnedMesh::draw(vk::CommandBuffer cb) const {
	auto const& v = vbo();
	cb.bindVertexBuffers(0, v.buffer, vk::DeviceSize{0});
	cb.bindVertexBuffers(1, m_joints.get().get().buffer, vk::DeviceSize{0});
	cb.bindVertexBuffers(2, m_weights.get().get().buffer, vk::DeviceSize{0});
	if (m_indices > 0) {
		auto const& i = ibo();
		cb.bindIndexBuffer(i.buffer, i.offset, vk::IndexType::eUint32);
		cb.drawIndexed(m_indices, 1u, 0u, 0u, 0u);
	} else {
		cb.draw(m_vertices, 1u, 0u, 0u);
	}
}

BufferView SkinnedMesh::vbo() const { return {.buffer = m_vibo.get().get().buffer, .size = m_vbo_size, .count = m_vertices}; }

BufferView SkinnedMesh::ibo() const {
	if (m_vbo_size == m_vibo.get().get().size) { return {}; }
	auto const& buffer = m_vibo.get().get();
	return {.buffer = buffer.buffer, .size = buffer.size - m_vbo_size, .offset = m_vbo_size, .count = m_indices};
}
} // namespace facade
