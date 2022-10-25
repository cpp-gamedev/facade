#pragma once
#include <facade/util/flex_array.hpp>
#include <facade/vk/gfx.hpp>
#include <vector>

namespace facade {
static constexpr std::size_t max_sets_v{16};
static constexpr std::size_t max_bindings_v{16};

struct SetLayout {
	FlexArray<vk::DescriptorSetLayoutBinding, max_bindings_v> bindings{};
	std::uint32_t set{};
};

class DescriptorSet {
  public:
	DescriptorSet(Gfx const& gfx, SetLayout const& layout, vk::DescriptorSet set, std::uint32_t number);

	void update(std::uint32_t binding, vk::ImageView image, vk::Sampler sampler, vk::ImageLayout layout) const;
	void update(std::uint32_t binding, vk::Buffer buffer, vk::DeviceSize size, vk::DeviceSize offset) const;

	void update(std::uint32_t binding, DescriptorImage const& image) const;
	void update(std::uint32_t binding, DescriptorBuffer const& buffer) const;
	void write(std::uint32_t binding, void const* data, std::size_t size);

	template <BufferWrite T>
	void write(std::uint32_t const binding, T const& t) {
		write(binding, std::addressof(t), sizeof(T));
	}

	vk::DescriptorSet set() const { return m_set; }
	void clear() { m_buffers.clear(); }

	std::uint32_t number() const { return m_number; }

  private:
	template <typename T>
	void update(T const& t, std::uint32_t binding, vk::DescriptorType type) const;

	template <typename... T>
	vk::DescriptorType get_type(std::uint32_t binding, T... match) const noexcept(false);

	SetLayout m_layout{};
	Gfx m_gfx{};
	std::vector<UniqueBuffer> m_buffers{};
	vk::DescriptorSet m_set{};
	std::uint32_t m_number{};
};
} // namespace facade
