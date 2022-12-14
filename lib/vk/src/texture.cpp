#include <facade/util/logger.hpp>
#include <facade/vk/cmd.hpp>
#include <facade/vk/texture.hpp>
#include <cmath>
#include <numeric>

namespace facade {
namespace {
ImageBarrier full_barrier(ImageView const& image, std::uint32_t levels, std::uint32_t layers, vk::ImageLayout const src, vk::ImageLayout const dst) {
	auto ret = ImageBarrier{};
	ret.barrier.srcAccessMask = ret.barrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite;
	ret.barrier.oldLayout = src;
	ret.barrier.newLayout = dst;
	ret.barrier.image = image.image;
	ret.barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, levels, 0, layers};
	ret.stages = {vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands};
	return ret;
}

struct MipMapWriter {
	vk::Image image;
	vk::Extent2D extent;
	vk::CommandBuffer cb;
	std::uint32_t mip_levels;

	std::uint32_t layer_count{1};
	ImageBarrier ib{};

	void blit_mips(std::uint32_t const src_level, vk::Offset3D const src_offset, vk::Offset3D const dst_offset) const {
		auto const src_isrl = vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, src_level, 0, layer_count};
		auto const dst_isrl = vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, src_level + 1, 0, layer_count};
		auto const region = vk::ImageBlit{src_isrl, {vk::Offset3D{}, src_offset}, dst_isrl, {vk::Offset3D{}, dst_offset}};
		cb.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, region, vk::Filter::eLinear);
	}

	void blit_next_mip(std::uint32_t const src_level, vk::Offset3D const src_offset, vk::Offset3D const dst_offset) {
		ib.barrier.oldLayout = vk::ImageLayout::eUndefined;
		ib.barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		ib.barrier.subresourceRange.baseMipLevel = src_level + 1;
		ib.transition(cb);

		blit_mips(src_level, src_offset, dst_offset);

		ib.barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		ib.barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		ib.transition(cb);
	}

	void operator()() {
		ib.barrier.image = image;
		ib.barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		ib.barrier.subresourceRange.baseArrayLayer = 0;
		ib.barrier.subresourceRange.layerCount = layer_count;
		ib.barrier.subresourceRange.baseMipLevel = 0;
		ib.barrier.subresourceRange.levelCount = 1;

		ib.barrier.srcAccessMask = ib.barrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite;
		ib.stages = {vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eTransfer};

		ib.barrier.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		ib.barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		ib.barrier.subresourceRange.baseMipLevel = 0;
		ib.transition(cb);

		ib.stages = {vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer};

		auto src_extent = vk::Extent3D{extent, 1};
		for (std::uint32_t mip = 0; mip + 1 < mip_levels; ++mip) {
			vk::Extent3D dst_extent = vk::Extent3D(std::max(src_extent.width / 2, 1u), std::max(src_extent.height / 2, 1u), 1u);
			auto const src_offset = vk::Offset3D{static_cast<int>(src_extent.width), static_cast<int>(src_extent.height), 1};
			auto const dst_offset = vk::Offset3D{static_cast<int>(dst_extent.width), static_cast<int>(dst_extent.height), 1};
			blit_next_mip(mip, src_offset, dst_offset);
			src_extent = dst_extent;
		}

		ib.barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		ib.barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		ib.barrier.subresourceRange.baseMipLevel = 0;
		ib.barrier.subresourceRange.levelCount = mip_levels;
		ib.stages.second = vk::PipelineStageFlagBits::eAllCommands;
		ib.transition(cb);
	}
};

bool can_mip(vk::PhysicalDevice const gpu, vk::Format const format) {
	static constexpr vk::FormatFeatureFlags flags_v{vk::FormatFeatureFlagBits::eBlitDst | vk::FormatFeatureFlagBits::eBlitSrc};
	auto const fsrc = gpu.getFormatProperties(format);
	return (fsrc.optimalTilingFeatures & flags_v) != vk::FormatFeatureFlags{};
}

Defer<UniqueImage> make_image(Gfx const& gfx, std::span<Image::View const> images, ImageCreateInfo const& info, vk::ImageViewType type) {
	auto const extent = vk::Extent2D{images[0].extent.x, images[0].extent.y};
	auto ret = Defer<UniqueImage>{gfx.vma.make_image(info, extent, type), gfx.shared->defer_queue};

	auto const accumulate_size = [](std::size_t total, Image::View const& i) { return total + i.bytes.size(); };
	auto const size = std::accumulate(images.begin(), images.end(), std::size_t{}, accumulate_size);
	auto staging = gfx.vma.make_buffer(vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst, size, true);
	if (!staging.get().buffer) {
		// TODO error
		return {};
	}

	auto* ptr = static_cast<std::byte*>(staging.get().ptr);
	for (auto const& image : images) {
		std::memcpy(ptr, image.bytes.data(), image.bytes.size());
		ptr += image.bytes.size();
	}

	auto isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, info.array_layers);
	auto icr = vk::ImageCopy(isrl, {}, isrl, {}, vk::Extent3D{extent, 1});
	auto bic = vk::BufferImageCopy({}, {}, {}, isrl, {}, icr.extent);
	auto cmd = Cmd{gfx};
	full_barrier(ret.get().get().image_view(), info.mip_levels, info.array_layers, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal)
		.transition(cmd.cb);
	cmd.cb.copyBufferToImage(staging.get().buffer, ret.get().get().image, vk::ImageLayout::eTransferDstOptimal, bic);
	full_barrier(ret.get().get().image_view(), info.mip_levels, info.array_layers, vk::ImageLayout::eTransferDstOptimal,
				 vk::ImageLayout::eShaderReadOnlyOptimal)
		.transition(cmd.cb);
	if (info.mip_levels > 1) { MipMapWriter{ret.get().get().image, ret.get().get().extent, cmd.cb, info.mip_levels}(); }
	return ret;
}
} // namespace

Sampler::Sampler(Gfx const& gfx, CreateInfo info) {
	auto sci = vk::SamplerCreateInfo{};
	sci.minFilter = info.min;
	sci.magFilter = info.mag;
	sci.anisotropyEnable = gfx.shared->device_limits.maxSamplerAnisotropy > 0.0f;
	sci.maxAnisotropy = gfx.shared->device_limits.maxSamplerAnisotropy;
	sci.borderColor = vk::BorderColor::eIntOpaqueBlack;
	sci.mipmapMode = vk::SamplerMipmapMode::eNearest;
	sci.addressModeU = info.mode_s;
	sci.addressModeV = info.mode_t;
	sci.addressModeW = info.mode_s;
	sci.maxLod = VK_LOD_CLAMP_NONE;
	m_sampler = {gfx.device.createSamplerUnique(sci), gfx.shared->defer_queue};
	m_name = std::move(info.name);
}

std::uint32_t Texture::mip_levels(vk::Extent2D extent) { return static_cast<std::uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1u; }

Texture::Texture(Gfx const& gfx, vk::Sampler sampler, Image::View image, CreateInfo info) : Texture(gfx, sampler, std::move(info.name)) {
	static constexpr auto magenta_v = FixedBitmap<1, 1>{0xff_B, 0x0_B, 0xff_B, 0xff_B};
	m_info.format = info.colour_space == ColourSpace::eLinear ? vk::Format::eR8G8B8A8Unorm : vk::Format::eR8G8B8A8Srgb;
	bool mip_mapped = info.mip_mapped;
	if (image.extent.x == 0 || image.extent.y == 0) {
		logger::warn("[Texture] invalid image extent: [0x0]");
		image = magenta_v;
		mip_mapped = false;
	}
	if (image.bytes.empty()) {
		logger::warn("[Texture] invalid image bytes: [empty]");
		image = magenta_v;
		mip_mapped = false;
	}
	if (mip_mapped && can_mip(m_gfx.gpu, m_info.format)) { m_info.mip_levels = mip_levels({image.extent.x, image.extent.y}); }
	m_image = make_image(m_gfx, {&image, 1}, m_info, vk::ImageViewType::e2D);
	m_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

Texture::Texture(Gfx const& gfx, vk::Sampler sampler, std::string name) : sampler(sampler), m_gfx(gfx), m_name(std::move(name)) {}

Cubemap::Cubemap(Gfx const& gfx, vk::Sampler sampler, std::span<Image::View const> images, std::string name) : Texture(gfx, sampler, std::move(name)) {
	assert(!images.empty());
	m_info.format = vk::Format::eR8G8B8A8Srgb;
	m_info.array_layers = 6;
	m_image = make_image(m_gfx, images, m_info, vk::ImageViewType::eCube);
	m_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}
} // namespace facade
