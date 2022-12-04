#include <facade/util/error.hpp>
#include <facade/util/flex_array.hpp>
#include <facade/vk/cmd.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/mesh_primitive.hpp>
#include <glm/mat4x4.hpp>

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

VertexLayout common_vertex_layout() {
	auto ret = VertexLayout{};
	// position
	ret.input.bindings.insert(vk::VertexInputBindingDescription{0, sizeof(glm::vec3)});
	ret.input.attributes.insert(vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat});
	// rgb
	ret.input.bindings.insert(vk::VertexInputBindingDescription{1, sizeof(glm::vec3)});
	ret.input.attributes.insert(vk::VertexInputAttributeDescription{1, 1, vk::Format::eR32G32B32Sfloat});
	// normal
	ret.input.bindings.insert(vk::VertexInputBindingDescription{2, sizeof(glm::vec3)});
	ret.input.attributes.insert(vk::VertexInputAttributeDescription{2, 2, vk::Format::eR32G32B32Sfloat});
	// uv
	ret.input.bindings.insert(vk::VertexInputBindingDescription{3, sizeof(glm::vec2)});
	ret.input.attributes.insert(vk::VertexInputAttributeDescription{3, 3, vk::Format::eR32G32Sfloat});
	return ret;
}

VertexLayout instanced_vertex_layout() {
	auto ret = common_vertex_layout();

	// instance matrix
	ret.input.bindings.insert(vk::VertexInputBindingDescription{6, sizeof(glm::mat4x4), vk::VertexInputRate::eInstance});
	ret.input.attributes.insert(vk::VertexInputAttributeDescription{6, 6, vk::Format::eR32G32B32A32Sfloat, 0 * sizeof(glm::vec4)});
	ret.input.attributes.insert(vk::VertexInputAttributeDescription{7, 6, vk::Format::eR32G32B32A32Sfloat, 1 * sizeof(glm::vec4)});
	ret.input.attributes.insert(vk::VertexInputAttributeDescription{8, 6, vk::Format::eR32G32B32A32Sfloat, 2 * sizeof(glm::vec4)});
	ret.input.attributes.insert(vk::VertexInputAttributeDescription{9, 6, vk::Format::eR32G32B32A32Sfloat, 3 * sizeof(glm::vec4)});

	ret.shader = "default.vert";

	return ret;
}

VertexLayout skinned_vertex_layout() {
	auto ret = common_vertex_layout();

	// joints
	ret.input.bindings.insert(vk::VertexInputBindingDescription{4, sizeof(glm::uvec4)});
	ret.input.attributes.insert(vk::VertexInputAttributeDescription{4, 4, vk::Format::eR32G32B32A32Uint});
	// weights
	ret.input.bindings.insert(vk::VertexInputBindingDescription{5, sizeof(glm::vec4)});
	ret.input.attributes.insert(vk::VertexInputAttributeDescription{5, 5, vk::Format::eR32G32B32A32Sfloat});

	ret.shader = "skinned.vert";

	return ret;
}
} // namespace

struct MeshPrimitive::Uploader {
	Gfx const& gfx;
	MeshPrimitive& out;

	void operator()(Geometry::Packed const& geometry, Joints joints) {
		assert(joints.joints.size() == joints.weights.size());
		{
			auto staging = FlexArray<UniqueBuffer, 2>{};
			auto cmd = Cmd{gfx, vk::PipelineStageFlagBits::eTopOfPipe};
			staging.insert(upload(cmd.cb, geometry));
			if (!joints.joints.empty()) { staging.insert(upload(cmd.cb, joints.joints, joints.weights)); }
		}
		out.m_vlayout = joints.joints.empty() ? instanced_vertex_layout() : skinned_vertex_layout();
		out.m_instance_binding = 6u;
	}

	[[nodiscard]] UniqueBuffer upload(vk::CommandBuffer cb, Geometry::Packed const& geometry) {
		auto const indices = std::span<std::uint32_t const>{geometry.indices};
		out.m_vertices = static_cast<std::uint32_t>(geometry.positions.size());
		out.m_indices = static_cast<std::uint32_t>(indices.size());
		auto const size = geometry.size_bytes();
		out.m_vibo.swap(gfx.vma.make_buffer(vi_flags_v, size, false));
		auto staging = gfx.vma.make_buffer(vk::BufferUsageFlagBits::eTransferSrc, size, true);
		auto writer = Writer{staging};
		out.m_offsets.positions = writer(std::span{geometry.positions});
		out.m_offsets.rgbs = writer(std::span{geometry.rgbs});
		out.m_offsets.normals = writer(std::span{geometry.normals});
		out.m_offsets.uvs = writer(std::span{geometry.uvs});
		if (!indices.empty()) { out.m_offsets.indices = writer(indices); }
		cb.copyBuffer(staging.get().buffer, out.m_vibo.get().get().buffer, vk::BufferCopy{{}, {}, size});
		return staging;
	}

	[[nodiscard]] UniqueBuffer upload(vk::CommandBuffer cb, std::span<glm::uvec4 const> joints, std::span<glm::vec4 const> weights) {
		assert(joints.size() >= weights.size());
		auto const size = joints.size_bytes() + weights.size_bytes();
		out.m_jwbo.swap(gfx.vma.make_buffer(v_flags_v, size, false));
		auto staging = gfx.vma.make_buffer(vk::BufferUsageFlagBits::eTransferSrc, size, true);
		auto writer = Writer{staging};
		out.m_offsets.joints = writer(joints);
		out.m_offsets.weights = writer(weights);
		cb.copyBuffer(staging.get().buffer, out.m_jwbo.get().get().buffer, vk::BufferCopy{{}, {}, size});
		return staging;
	}
};

MeshPrimitive::MeshPrimitive(Gfx const& gfx, Geometry::Packed const& geometry, Joints joints, std::string name)
	: m_vibo(gfx.shared->defer_queue), m_jwbo(gfx.shared->defer_queue), m_name(std::move(name)) {
	Uploader{gfx, *this}(geometry, joints);
}

MeshPrimitive::MeshPrimitive(Gfx const& gfx, Geometry const& geometry, Joints joints, std::string name)
	: MeshPrimitive{gfx, Geometry::Packed::from(geometry), joints, std::move(name)} {}

auto MeshPrimitive::info() const -> Info { return {m_vertices, m_indices}; }

void MeshPrimitive::draw(vk::CommandBuffer cb, std::uint32_t instances) const {
	auto const& v = m_vibo.get().get();
	vk::Buffer const buffers[] = {v.buffer, v.buffer, v.buffer, v.buffer};
	vk::DeviceSize const offsets[] = {m_offsets.positions, m_offsets.rgbs, m_offsets.normals, m_offsets.uvs};
	cb.bindVertexBuffers(0u, buffers, offsets);
	if (m_jwbo.get().get().size > 0) {
		vk::Buffer const buffers[] = {m_jwbo.get().get().buffer, m_jwbo.get().get().buffer};
		vk::DeviceSize const offsets[] = {m_offsets.joints, m_offsets.weights};
		cb.bindVertexBuffers(4u, buffers, offsets);
	}
	if (m_indices > 0) {
		cb.bindIndexBuffer(v.buffer, m_offsets.indices, vk::IndexType::eUint32);
		cb.drawIndexed(m_indices, instances, 0u, 0u, 0u);
	} else {
		cb.draw(m_vertices, instances, 0u, 0u);
	}
}
} // namespace facade
