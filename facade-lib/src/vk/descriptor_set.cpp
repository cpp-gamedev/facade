#include <facade/util/error.hpp>
#include <facade/util/string.hpp>
#include <facade/vk/buffer.hpp>
#include <facade/vk/descriptor_set.hpp>
#include <facade/vk/texture.hpp>

namespace facade {
namespace {
constexpr vk::BufferUsageFlags get_usage(vk::DescriptorType const type) {
	switch (type) {
	default:
	case vk::DescriptorType::eUniformBuffer: return vk::BufferUsageFlagBits::eUniformBuffer;
	case vk::DescriptorType::eStorageBuffer: return vk::BufferUsageFlagBits::eStorageBuffer;
	}
}
} // namespace

DescriptorSet::DescriptorSet(Gfx const& gfx, SetLayout const& layout, vk::DescriptorSet set, std::uint32_t number)
	: m_layout{layout}, m_gfx{gfx}, m_set{set}, m_number{number} {}

void DescriptorSet::update(std::uint32_t binding, vk::ImageView image, vk::Sampler sampler, vk::ImageLayout layout) const {
	auto const type = get_type(binding, vk::DescriptorType::eCombinedImageSampler);
	update(vk::DescriptorImageInfo{sampler, image, layout}, binding, type);
}

void DescriptorSet::update(std::uint32_t binding, vk::Buffer buffer, vk::DeviceSize size, vk::DeviceSize offset) const {
	auto const type = get_type(binding, vk::DescriptorType::eUniformBuffer, vk::DescriptorType::eStorageBuffer);
	update(vk::DescriptorBufferInfo{buffer, offset, size}, binding, type);
}

void DescriptorSet::update(std::uint32_t binding, DescriptorImage const& image) const {
	update(binding, image.image, image.sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void DescriptorSet::update(std::uint32_t binding, DescriptorBuffer const& buffer) const {
	auto const type = get_type(binding, buffer.type);
	update(vk::DescriptorBufferInfo{buffer.buffer, 0, buffer.size}, binding, type);
}

void DescriptorSet::write(std::uint32_t binding, void const* data, std::size_t size) {
	auto const type = get_type(binding, vk::DescriptorType::eUniformBuffer, vk::DescriptorType::eStorageBuffer);
	auto& buffer = m_buffers.emplace_back();
	buffer = m_gfx.vma.make_buffer(get_usage(type), size, true);
	std::memcpy(buffer.get().ptr, data, size);
	update(vk::DescriptorBufferInfo{buffer.get().buffer, {}, buffer.get().size}, binding, type);
}

template <typename T>
void DescriptorSet::update(T const& t, std::uint32_t binding, vk::DescriptorType type) const {
	auto wds = vk::WriteDescriptorSet{m_set, binding, 0, 1, type};
	if constexpr (std::same_as<T, vk::DescriptorBufferInfo>) {
		wds.pBufferInfo = &t;
	} else {
		wds.pImageInfo = &t;
	}
	m_gfx.device.updateDescriptorSets(wds, {});
}

template <typename... T>
vk::DescriptorType DescriptorSet::get_type(std::uint32_t const binding, T... match) const noexcept(false) {
	if (binding >= m_layout.bindings.span().size()) { throw Error{concat("DescriptorSet: Invalid binding: ", binding)}; }
	auto const& bind = m_layout.bindings.span()[binding];
	if ((... && (bind.descriptorType != match))) { throw Error{"DescriptorSet: Invalid descriptor type"}; }
	return bind.descriptorType;
}
} // namespace facade
