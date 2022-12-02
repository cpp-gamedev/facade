#include <facade/util/error.hpp>
#include <facade/vk/cmd.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/skinned_mesh.hpp>

namespace facade {
namespace {
constexpr auto v_flags_v = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
constexpr auto vi_flags_v = v_flags_v | vk::BufferUsageFlagBits::eIndexBuffer;
} // namespace

SkinnedMesh::SkinnedMesh(Gfx const& gfx, Geometry::Packed const& geometry, CreateInfo const& create_info, std::string name)
	: m_vibo(gfx.shared->defer_queue), m_joints(gfx.shared->defer_queue), m_weights(gfx.shared->defer_queue), m_name(std::move(name)) {
	if (create_info.joints.size() != create_info.weights.size()) { throw Error{"Mismatched joints and weights"}; }

	auto const indices = std::span<std::uint32_t const>{geometry.indices};
	assert(geometry.positions.size() == geometry.rgbs.size() && geometry.positions.size() == geometry.normals.size() &&
		   geometry.positions.size() == geometry.uvs.size());
	m_vertices = static_cast<std::uint32_t>(geometry.positions.size());
	m_indices = static_cast<std::uint32_t>(indices.size());
	m_vbo_size = geometry.positions.size() * (sizeof(geometry.positions[0]) + sizeof(geometry.rgbs[0]) + sizeof(geometry.normals[0]) + sizeof(geometry.uvs[0]));
	auto const size = m_vbo_size + indices.size_bytes();
	m_vibo.swap(gfx.vma.make_buffer(vi_flags_v, size, false));
	auto staging = gfx.vma.make_buffer(vk::BufferUsageFlagBits::eTransferSrc, size, true);
	auto s = std::span{geometry.positions}.size_bytes();
	auto o = std::size_t{};
	m_offsets.positions = 0u;
	std::memcpy(static_cast<std::byte*>(staging.get().ptr) + o, geometry.positions.data(), s);
	o += s;
	m_offsets.rgbs = o;
	s = std::span{geometry.rgbs}.size_bytes();
	std::memcpy(static_cast<std::byte*>(staging.get().ptr) + o, geometry.rgbs.data(), s);
	o += s;
	m_offsets.normals = o;
	s = std::span{geometry.normals}.size_bytes();
	std::memcpy(static_cast<std::byte*>(staging.get().ptr) + o, geometry.normals.data(), s);
	o += s;
	m_offsets.uvs = o;
	s = std::span{geometry.uvs}.size_bytes();
	std::memcpy(static_cast<std::byte*>(staging.get().ptr) + o, geometry.uvs.data(), s);
	if (!indices.empty()) { std::memcpy(static_cast<std::byte*>(staging.get().ptr) + m_vbo_size, indices.data(), indices.size_bytes()); }
	auto const bc = vk::BufferCopy{{}, {}, size};
	auto cmd = Cmd{gfx, vk::PipelineStageFlagBits::eTopOfPipe};
	cmd.cb.copyBuffer(staging.get().buffer, m_vibo.get().get().buffer, bc);

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
	vk::Buffer const buffers[] = {v.buffer, v.buffer, v.buffer, v.buffer};
	vk::DeviceSize const offsets[] = {m_offsets.positions, m_offsets.rgbs, m_offsets.normals, m_offsets.uvs};
	cb.bindVertexBuffers(0u, buffers, offsets);
	cb.bindVertexBuffers(4u, m_joints.get().get().buffer, vk::DeviceSize{0});
	cb.bindVertexBuffers(5u, m_weights.get().get().buffer, vk::DeviceSize{0});
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
