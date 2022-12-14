#pragma once
#include <facade/vk/defer.hpp>
#include <facade/vk/gfx.hpp>
#include <facade/vk/rotator.hpp>
#include <span>
#include <vector>

namespace facade {
template <typename T>
concept Writable = std::is_trivially_copyable_v<T>;

class Buffer {
  public:
	class Pool;

	template <typename Type>
	class VecPool;

	enum class Type : std::uint8_t { eUniform, eVertexIndex, eStorage, eInstance };

	static constexpr vk::DescriptorType to_descriptor_type(Buffer::Type const type) {
		switch (type) {
		default:
		case Buffer::Type::eUniform: return vk::DescriptorType::eUniformBuffer;
		case Buffer::Type::eStorage: return vk::DescriptorType::eStorageBuffer;
		}
	}

	explicit Buffer(Gfx const& gfx, Type type);

	template <Writable T>
	void write(std::span<T> elements) {
		write(elements.data(), elements.size_bytes(), elements.size());
	}

	void rotate();

	void refresh() const;

	BufferView view() const;
	DescriptorBuffer descriptor_buffer() const;

	Type type() const { return m_type; }

  private:
	void write(void const* data, std::size_t size, std::size_t count);

	Gfx m_gfx{};
	mutable Rotator<Defer<UniqueBuffer>> m_buffers{};
	std::vector<std::byte> m_data{};
	std::uint32_t m_count{};
	Type m_type{};
};

class Buffer::Pool {
  public:
	explicit Pool(Buffer::Type type) : m_type(type) {}

	Buffer& get(Gfx const& gfx);
	void rotate();

  private:
	std::vector<Buffer> m_buffers{};
	std::size_t m_index{};
	Buffer::Type m_type{};
};

template <typename T>
class Buffer::VecPool : public Pool {
  public:
	explicit VecPool(Buffer::Type type) : Pool(type) {}

	template <typename F>
	Buffer const& rewrite(Gfx const& gfx, std::size_t reserve, F func) {
		clear(reserve);
		func(m_vec);
		return update(gfx);
	}

  private:
	void clear(std::size_t reserve) {
		m_vec.clear();
		m_vec.reserve(reserve);
	}

	Buffer const& update(Gfx const& gfx) {
		auto& ret = get(gfx);
		ret.write(std::span{m_vec});
		return ret;
	}

	std::vector<T> m_vec{};
};
} // namespace facade
