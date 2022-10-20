#pragma once
#include <vk_mem_alloc.h>
#include <facade/util/unique.hpp>
#include <vulkan/vulkan.hpp>

namespace facade {
template <typename T, typename U = T>
using Pair = std::pair<T, U>;

template <typename Type>
concept BufferWrite = !std::is_pointer_v<Type> && std::is_trivially_destructible_v<Type>;

constexpr vk::Format srgb_formats_v[] = {vk::Format::eR8G8B8A8Srgb, vk::Format::eB8G8R8A8Srgb};
constexpr vk::Format linear_formats_v[] = {vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8A8Unorm};

constexpr bool is_linear(vk::Format format) {
	return std::find(std::begin(linear_formats_v), std::end(linear_formats_v), format) != std::end(linear_formats_v);
}

constexpr bool is_srgb(vk::Format format) { return std::find(std::begin(srgb_formats_v), std::end(srgb_formats_v), format) != std::end(srgb_formats_v); }

struct BufferView {
	vk::Buffer buffer{};
	vk::DeviceSize size{};
	vk::DeviceSize offset{};
};

struct DescriptorBuffer {
	vk::Buffer buffer{};
	vk::DeviceSize size{};
	vk::DescriptorType type{};
};

struct ImageView {
	vk::Image image{};
	vk::ImageView view{};
	vk::Extent2D extent{};
};

struct DescriptorImage {
	vk::ImageView image{};
	vk::Sampler sampler{};
};

struct MeshView {
	BufferView vertices{};
	BufferView indices{};
	std::uint32_t vertex_count{};
	std::uint32_t index_count{};
};

struct ImageCreateInfo {
	static constexpr auto usage_v = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

	vk::Format format{vk::Format::eR8G8B8A8Srgb};
	vk::ImageUsageFlags usage{usage_v};
	vk::ImageAspectFlagBits aspect{vk::ImageAspectFlagBits::eColor};
	vk::ImageTiling tiling{vk::ImageTiling::eOptimal};
	std::uint32_t mip_levels{1};
	std::uint32_t array_layers{1};
	vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
};

struct ImageBarrier {
	vk::ImageMemoryBarrier barrier{};
	Pair<vk::PipelineStageFlags> stages{};

	void transition(vk::CommandBuffer cb) const { cb.pipelineBarrier(stages.first, stages.second, {}, {}, {}, barrier); }
};

struct Vma {
	struct Deleter;
	struct Allocation;
	struct Buffer;
	struct Image;

	struct Mls {
		std::uint32_t mip_levels{1};
		std::uint32_t array_layers{1};
		vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
	};

	static constexpr auto isr_v = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

	vk::Device device{};
	VmaAllocator allocator{};

	Unique<Buffer, Deleter> make_buffer(vk::BufferUsageFlags usage, vk::DeviceSize size, bool host_visible) const;
	Unique<Image, Deleter> make_image(ImageCreateInfo const& info, vk::Extent2D extent) const;
	vk::UniqueImageView make_image_view(vk::Image const image, vk::Format const format, vk::ImageSubresourceRange isr = isr_v) const;
};

struct Vma::Allocation {
	Vma vma{};
	VmaAllocation allocation{};
};

struct Vma::Deleter {
	void operator()(Vma const& vma) const;
	void operator()(Buffer const& buffer) const;
	void operator()(Image const& image) const;
};

using UniqueVma = Unique<Vma, Vma::Deleter>;

struct Vma::Buffer {
	Allocation allocation{};
	vk::Buffer buffer{};
	vk::DeviceSize size{};
	void* ptr{};
};

struct Vma::Image {
	Allocation allocation{};
	vk::Image image{};
	vk::UniqueImageView view{};
	vk::Extent2D extent{};

	ImageView image_view() const { return {image, *view, extent}; }
};

using UniqueBuffer = Unique<Vma::Buffer, Vma::Deleter>;
using UniqueImage = Unique<Vma::Image, Vma::Deleter>;
} // namespace facade
