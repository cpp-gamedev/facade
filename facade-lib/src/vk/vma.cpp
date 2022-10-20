#include <facade/util/error.hpp>
#include <facade/vk/vma.hpp>

namespace facade {
void Vma::Deleter::operator()(Vma const& vma) const {
	if (vma.allocator) { vmaDestroyAllocator(vma.allocator); }
}

void Vma::Deleter::operator()(Buffer const& buffer) const {
	auto const& allocation = buffer.allocation;
	if (buffer.buffer && allocation.allocation) {
		if (buffer.ptr) { vmaUnmapMemory(allocation.vma.allocator, allocation.allocation); }
		vmaDestroyBuffer(allocation.vma.allocator, buffer.buffer, allocation.allocation);
	}
}

void Vma::Deleter::operator()(Image const& image) const {
	auto const& allocation = image.allocation;
	if (image.image && allocation.allocation) { vmaDestroyImage(allocation.vma.allocator, image.image, allocation.allocation); }
}

UniqueBuffer Vma::make_buffer(vk::BufferUsageFlags const usage, vk::DeviceSize const size, bool host_visible) const {
	auto vaci = VmaAllocationCreateInfo{};
	vaci.usage = VMA_MEMORY_USAGE_AUTO;
	if (host_visible) { vaci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT; }
	auto const bci = vk::BufferCreateInfo{{}, size, usage};
	auto vbci = static_cast<VkBufferCreateInfo>(bci);
	auto buffer = VkBuffer{};
	auto ret = Buffer{};
	if (vmaCreateBuffer(allocator, &vbci, &vaci, &buffer, &ret.allocation.allocation, {}) != VK_SUCCESS) { throw Error{"Failed to allocate Vulkan Buffer"}; }
	ret.buffer = buffer;
	ret.allocation.vma = *this;
	ret.size = bci.size;
	if (host_visible) { vmaMapMemory(allocator, ret.allocation.allocation, &ret.ptr); }
	return ret;
}

UniqueImage Vma::make_image(ImageCreateInfo const& info, vk::Extent2D const extent) const {
	if (extent.width == 0 || extent.height == 0) { throw Error{"Attempt to allocate 0-sized image"}; }
	auto vaci = VmaAllocationCreateInfo{};
	vaci.usage = VMA_MEMORY_USAGE_AUTO;
	auto ici = vk::ImageCreateInfo{};
	ici.usage = info.usage;
	ici.imageType = vk::ImageType::e2D;
	ici.tiling = info.tiling;
	ici.arrayLayers = info.array_layers;
	ici.mipLevels = info.mip_levels;
	ici.extent = vk::Extent3D{extent, 1};
	ici.format = info.format;
	ici.samples = info.samples;
	auto const vici = static_cast<VkImageCreateInfo>(ici);
	auto image = VkImage{};
	auto ret = Image{};
	if (vmaCreateImage(allocator, &vici, &vaci, &image, &ret.allocation.allocation, {}) != VK_SUCCESS) { throw Error{"Failed to allocate Vulkan Image"}; }
	ret.view = make_image_view(image, info.format, {info.aspect, 0, info.mip_levels, 0, info.array_layers});
	ret.image = image;
	ret.allocation.vma = *this;
	ret.extent = extent;
	return ret;
}

vk::UniqueImageView Vma::make_image_view(vk::Image const image, vk::Format const format, vk::ImageSubresourceRange isr) const {
	vk::ImageViewCreateInfo info;
	info.viewType = vk::ImageViewType::e2D;
	info.format = format;
	info.components.r = info.components.g = info.components.b = info.components.a = vk::ComponentSwizzle::eIdentity;
	info.subresourceRange = isr;
	info.image = image;
	return device.createImageViewUnique(info);
}
} // namespace facade
