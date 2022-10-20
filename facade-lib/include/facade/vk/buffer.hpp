#pragma once
#include <facade/util/ring_buffer.hpp>
#include <facade/vk/gfx.hpp>
#include <vector>

namespace facade {
class Buffer {
  public:
	enum class Type : std::uint8_t { eUniform, eVertexIndex, eStorage };

	static constexpr vk::DescriptorType to_descriptor_type(Buffer::Type const type) {
		switch (type) {
		default:
		case Buffer::Type::eUniform: return vk::DescriptorType::eUniformBuffer;
		case Buffer::Type::eStorage: return vk::DescriptorType::eStorageBuffer;
		}
	}

	explicit Buffer(Gfx const& gfx, Type type);

	void write(void const* data, std::size_t size);
	void rotate();

	void refresh() const;

	BufferView view() const;
	DescriptorBuffer descriptor_buffer() const;

	Type type() const { return m_type; }

  private:
	Gfx m_gfx{};
	mutable RingBuffer<UniqueBuffer> m_buffers{};
	std::vector<std::byte> m_data{};
	std::size_t m_size{};
	Type m_type{};
};
} // namespace facade
