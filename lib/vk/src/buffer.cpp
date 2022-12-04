#include <facade/util/error.hpp>
#include <facade/util/ptr.hpp>
#include <facade/vk/buffer.hpp>
#include <facade/vk/cmd.hpp>

namespace facade {
namespace {
constexpr vk::BufferUsageFlags get_usage(Buffer::Type const type) {
	switch (type) {
	case Buffer::Type::eUniform: return vk::BufferUsageFlagBits::eUniformBuffer;
	case Buffer::Type::eVertexIndex: return vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer;
	case Buffer::Type::eStorage: return vk::BufferUsageFlagBits::eStorageBuffer;
	case Buffer::Type::eInstance: return vk::BufferUsageFlagBits::eVertexBuffer;
	}
	throw Error{"Unknown type"};
}
} // namespace

Buffer::Buffer(Gfx const& gfx, Type type) : m_gfx{gfx}, m_type{type} {}

void Buffer::write(void const* data, std::size_t const size, std::size_t const count) {
	m_data.clear();
	if (size > 0) {
		m_data.resize(size);
		std::memcpy(m_data.data(), data, size);
		assert(count > 0 && count <= size);
		m_count = static_cast<std::uint32_t>(count);
	} else {
		m_data.push_back({});
		m_count = 1u;
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
	return {.buffer = m_buffers.get().get().get().buffer, .size = m_data.size(), .count = m_count};
}

DescriptorBuffer Buffer::descriptor_buffer() const {
	refresh();
	return {m_buffers.get().get().get().buffer, m_data.size(), to_descriptor_type(m_type)};
}

Buffer& Buffer::Pool::get(Gfx const& gfx) {
	auto ret = Ptr<Buffer>{};
	if (m_index < m_buffers.size()) {
		ret = &m_buffers[m_index++];
	} else {
		++m_index;
		ret = &m_buffers.emplace_back(gfx, m_type);
	}
	return *ret;
}

void Buffer::Pool::rotate() {
	for (auto& buffer : m_buffers) { buffer.rotate(); }
	m_index = 0;
}
} // namespace facade
