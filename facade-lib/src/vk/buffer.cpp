#include <facade/vk/buffer.hpp>
#include <facade/vk/cmd.hpp>

namespace facade {
namespace {
constexpr vk::BufferUsageFlags get_usage(Buffer::Type const type) {
	switch (type) {
	default:
	case Buffer::Type::eUniform: return vk::BufferUsageFlagBits::eUniformBuffer;
	case Buffer::Type::eVertexIndex: return vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer;
	case Buffer::Type::eStorage: return vk::BufferUsageFlagBits::eStorageBuffer;
	}
}
} // namespace

Buffer::Buffer(Gfx const& gfx, Type type) : m_gfx{gfx}, m_type{type} {}

void Buffer::write(void const* data, std::size_t size) {
	m_data.clear();
	if (size > 0) {
		m_data.resize(size);
		std::memcpy(m_data.data(), data, size);
	} else {
		m_data.push_back({});
	}
}

void Buffer::rotate() { m_buffers.rotate(); }

void Buffer::refresh() const {
	auto& buffer = m_buffers.get();
	if (m_data.size() > buffer.get().get().size) { buffer = {m_gfx.vma.make_buffer(get_usage(m_type), m_data.size(), true), m_gfx.shared->defer_queue}; }
	std::memcpy(buffer.get().get().ptr, m_data.data(), m_data.size());
}

BufferView Buffer::view() const {
	refresh();
	return {m_buffers.get().get().get().buffer, m_size};
}

DescriptorBuffer Buffer::descriptor_buffer() const {
	refresh();
	return {m_buffers.get().get().get().buffer, m_buffers.get().get().get().size, to_descriptor_type(m_type)};
}
} // namespace facade
