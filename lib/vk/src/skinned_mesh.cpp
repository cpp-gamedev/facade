#include <facade/util/error.hpp>
#include <facade/util/flex_array.hpp>
#include <facade/vk/cmd.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/skinned_mesh.hpp>

namespace facade {
namespace {
constexpr auto v_flags_v = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
constexpr auto vi_flags_v = v_flags_v | vk::BufferUsageFlagBits::eIndexBuffer;

struct Writer {
	UniqueBuffer& out;
	std::size_t offset{};

	template <typename T>
	std::size_t operator()(std::span<T> data) {
		auto ret = offset;
		std::memcpy(static_cast<std::byte*>(out.get().ptr) + offset, data.data(), data.size_bytes());
		offset += data.size_bytes();
		return ret;
	}
};
} // namespace

SkinnedMesh::SkinnedMesh(Gfx const& gfx, Geometry::Packed const& geometry, CreateInfo const& create_info, std::string name)
	: m_vibo(gfx.shared->defer_queue), m_jwbo(gfx.shared->defer_queue), m_name(std::move(name)) {
	if (create_info.joints.size() != create_info.weights.size()) { throw Error{"Mismatched joints and weights"}; }

	auto const indices = std::span<std::uint32_t const>{geometry.indices};
	m_vertices = static_cast<std::uint32_t>(geometry.positions.size());
	m_indices = static_cast<std::uint32_t>(indices.size());
	auto const size = geometry.size_bytes();
	m_vibo.swap(gfx.vma.make_buffer(vi_flags_v, size, false));
	auto staging = FlexArray<UniqueBuffer, 2>{};

	auto& s0 = staging.insert(gfx.vma.make_buffer(vk::BufferUsageFlagBits::eTransferSrc, size, true));
	auto writer = Writer{s0};
	m_offsets.positions = writer(std::span{geometry.positions});
	m_offsets.rgbs = writer(std::span{geometry.rgbs});
	m_offsets.normals = writer(std::span{geometry.normals});
	m_offsets.uvs = writer(std::span{geometry.uvs});
	if (!indices.empty()) { m_offsets.indices = writer(indices); }

	auto const bc = vk::BufferCopy{{}, {}, size};
	auto cmd = Cmd{gfx, vk::PipelineStageFlagBits::eTopOfPipe};
	cmd.cb.copyBuffer(s0.get().buffer, m_vibo.get().get().buffer, bc);

	if (!create_info.joints.empty()) {
		auto const size = create_info.joints.size_bytes() + create_info.weights.size_bytes();
		m_jwbo.swap(gfx.vma.make_buffer(v_flags_v, size, false));
		auto& s1 = staging.insert(gfx.vma.make_buffer(vk::BufferUsageFlagBits::eTransferSrc, size, true));
		auto writer = Writer{s1};
		m_offsets.joints = writer(create_info.joints);
		m_offsets.weights = writer(create_info.weights);
		cmd.cb.copyBuffer(s1.get().buffer, m_jwbo.get().get().buffer, vk::BufferCopy{{}, {}, size});
	}
}

auto SkinnedMesh::info() const -> Info { return {m_vertices, m_indices}; }

void SkinnedMesh::draw(vk::CommandBuffer cb, std::uint32_t instances) const {
	auto const& v = m_vibo.get().get();
	vk::Buffer const verts[] = {v.buffer, v.buffer, v.buffer, v.buffer};
	vk::DeviceSize const vert_offsets[] = {m_offsets.positions, m_offsets.rgbs, m_offsets.normals, m_offsets.uvs};
	cb.bindVertexBuffers(0u, verts, vert_offsets);
	if (has_joints()) {
		vk::Buffer const jw[] = {m_jwbo.get().get().buffer, m_jwbo.get().get().buffer};
		vk::DeviceSize const jw_offsets[] = {m_offsets.joints, m_offsets.weights};
		cb.bindVertexBuffers(4u, jw, jw_offsets);
	}
	if (m_indices > 0) {
		cb.bindIndexBuffer(v.buffer, m_offsets.indices, vk::IndexType::eUint32);
		cb.drawIndexed(m_indices, instances, 0u, 0u, 0u);
	} else {
		cb.draw(m_vertices, instances, 0u, 0u);
	}
}
} // namespace facade
