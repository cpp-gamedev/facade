#include <facade/util/error.hpp>
#include <facade/util/flex_array.hpp>
#include <facade/vk/cmd.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/skinned_mesh.hpp>

namespace facade {
namespace {
constexpr auto v_flags_v = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
constexpr auto vi_flags_v = v_flags_v | vk::BufferUsageFlagBits::eIndexBuffer;
} // namespace

SkinnedMesh::SkinnedMesh(Gfx const& gfx, Geometry::Packed const& geometry, CreateInfo const& create_info, std::string name)
	: m_vibo(gfx.shared->defer_queue), m_jwbo(gfx.shared->defer_queue), m_name(std::move(name)) {
	if (create_info.joints.size() != create_info.weights.size()) { throw Error{"Mismatched joints and weights"}; }

	auto const indices = std::span<std::uint32_t const>{geometry.indices};
	assert(geometry.positions.size() == geometry.rgbs.size() && geometry.positions.size() == geometry.normals.size() &&
		   geometry.positions.size() == geometry.uvs.size());
	m_vertices = static_cast<std::uint32_t>(geometry.positions.size());
	m_indices = static_cast<std::uint32_t>(indices.size());
	m_offsets.indices =
		geometry.positions.size() * (sizeof(geometry.positions[0]) + sizeof(geometry.rgbs[0]) + sizeof(geometry.normals[0]) + sizeof(geometry.uvs[0]));
	auto const size = m_offsets.indices + indices.size_bytes();
	m_vibo.swap(gfx.vma.make_buffer(vi_flags_v, size, false));
	auto staging = FlexArray<UniqueBuffer, 2>{};

	auto& s0 = staging.insert(gfx.vma.make_buffer(vk::BufferUsageFlagBits::eTransferSrc, size, true));
	auto s = std::span{geometry.positions}.size_bytes();
	auto o = std::size_t{};
	m_offsets.positions = 0u;
	std::memcpy(static_cast<std::byte*>(s0.get().ptr) + o, geometry.positions.data(), s);
	o += s;
	m_offsets.rgbs = o;
	s = std::span{geometry.rgbs}.size_bytes();
	std::memcpy(static_cast<std::byte*>(s0.get().ptr) + o, geometry.rgbs.data(), s);
	o += s;
	m_offsets.normals = o;
	s = std::span{geometry.normals}.size_bytes();
	std::memcpy(static_cast<std::byte*>(s0.get().ptr) + o, geometry.normals.data(), s);
	o += s;
	m_offsets.uvs = o;
	s = std::span{geometry.uvs}.size_bytes();
	std::memcpy(static_cast<std::byte*>(s0.get().ptr) + o, geometry.uvs.data(), s);
	if (!indices.empty()) { std::memcpy(static_cast<std::byte*>(s0.get().ptr) + m_offsets.indices, indices.data(), indices.size_bytes()); }
	auto const bc = vk::BufferCopy{{}, {}, size};
	auto cmd = Cmd{gfx, vk::PipelineStageFlagBits::eTopOfPipe};
	cmd.cb.copyBuffer(s0.get().buffer, m_vibo.get().get().buffer, bc);

	if (!create_info.joints.empty()) {
		auto const size = create_info.joints.size_bytes() + create_info.weights.size_bytes();
		m_jwbo.swap(gfx.vma.make_buffer(v_flags_v, size, false));
		m_offsets.weights = create_info.joints.size_bytes();
		auto& s1 = staging.insert(gfx.vma.make_buffer(vk::BufferUsageFlagBits::eTransferSrc, size, true));
		std::memcpy(s1.get().ptr, create_info.joints.data(), create_info.joints.size_bytes());
		std::memcpy(static_cast<std::byte*>(s1.get().ptr) + m_offsets.weights, create_info.weights.data(), create_info.weights.size_bytes());
		cmd.cb.copyBuffer(s1.get().buffer, m_jwbo.get().get().buffer, vk::BufferCopy{{}, {}, size});
	}
}

auto SkinnedMesh::info() const -> Info { return {m_vertices, m_indices}; }

void SkinnedMesh::draw(vk::CommandBuffer cb) const {
	auto const& v = m_vibo.get().get();
	vk::Buffer const verts[] = {v.buffer, v.buffer, v.buffer, v.buffer};
	vk::DeviceSize const vert_offsets[] = {m_offsets.positions, m_offsets.rgbs, m_offsets.normals, m_offsets.uvs};
	cb.bindVertexBuffers(0u, verts, vert_offsets);
	vk::Buffer const jw[] = {m_jwbo.get().get().buffer, m_jwbo.get().get().buffer};
	vk::DeviceSize const jw_offsets[] = {m_offsets.joints, m_offsets.weights};
	cb.bindVertexBuffers(4u, jw, jw_offsets);
	if (m_indices > 0) {
		cb.bindIndexBuffer(v.buffer, m_offsets.indices, vk::IndexType::eUint32);
		cb.drawIndexed(m_indices, 1u, 0u, 0u, 0u);
	} else {
		cb.draw(m_vertices, 1u, 0u, 0u);
	}
}
} // namespace facade
