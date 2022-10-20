#include <facade/util/logger.hpp>
#include <facade/vk/cmd.hpp>
#include <facade/vk/texture.hpp>
#include <cmath>

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
			vk::Extent3D dst_extent = vk::Extent3D(std::max(src_extent.width / 2, 1U), std::max(src_extent.height / 2, 1U), 1U);
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
} // namespace

Sampler::Sampler(CreateInfo const& info) {
	auto sci = vk::SamplerCreateInfo{};
	sci.minFilter = info.min;
	sci.magFilter = info.mag;
	sci.anisotropyEnable = info.gfx.shared->device_limits.maxSamplerAnisotropy > 0.0f;
	sci.maxAnisotropy = info.gfx.shared->device_limits.maxSamplerAnisotropy;
	sci.borderColor = vk::BorderColor::eIntOpaqueBlack;
	sci.mipmapMode = vk::SamplerMipmapMode::eNearest;
	sci.addressModeU = info.mode_s;
	sci.addressModeV = info.mode_t;
	sci.addressModeW = info.mode_s;
	sci.maxLod = VK_LOD_CLAMP_NONE;
	m_sampler = {info.gfx.device.createSamplerUnique(sci), info.gfx.shared->defer_queue};
}

std::uint32_t Texture::mip_levels(vk::Extent2D extent) { return static_cast<std::uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1U; }

vk::UniqueSampler Texture::make_sampler(Gfx const& gfx, vk::SamplerAddressMode const mode, vk::Filter const filter) {
	auto sci = vk::SamplerCreateInfo{};
	sci.minFilter = sci.magFilter = filter;
	sci.anisotropyEnable = gfx.shared->device_limits.maxSamplerAnisotropy > 0.0f;
	sci.maxAnisotropy = gfx.shared->device_limits.maxSamplerAnisotropy;
	sci.borderColor = vk::BorderColor::eIntOpaqueBlack;
	sci.mipmapMode = vk::SamplerMipmapMode::eNearest;
	sci.addressModeU = sci.addressModeV = sci.addressModeW = mode;
	sci.maxLod = VK_LOD_CLAMP_NONE;
	return gfx.device.createSamplerUnique(sci);
}

Texture::Texture(CreateInfo const& info, Image::View image) : sampler{info.sampler}, m_gfx{info.gfx} {
	static constexpr std::uint8_t magenta_v[] = {0xff, 0x0, 0xff, 0xff};
	m_info.format = info.colour_space == ColourSpace::eLinear ? vk::Format::eR8G8B8A8Unorm : vk::Format::eR8G8B8A8Srgb;
	bool mip_mapped = info.mip_mapped;
	if (image.extent.x == 0 || image.extent.y == 0) {
		logger::warn("[Texture] invalid image extent: [0x0]");
		image.extent = {1, 1};
		image.bytes = {reinterpret_cast<std::byte const*>(magenta_v), std::size(magenta_v)};
		mip_mapped = false;
	}
	auto const extent = vk::Extent2D{image.extent.x, image.extent.y};
	if (mip_mapped && can_mip(m_gfx.gpu, m_info.format)) { m_info.mip_levels = mip_levels(extent); }
	m_image = {m_gfx.vma.make_image(m_info, extent), m_gfx.shared->defer_queue};

	auto staging = m_gfx.vma.make_buffer(vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst, image.bytes.size(), true);
	if (!staging.get().buffer) {
		// TODO error
		return;
	}

	std::memcpy(staging.get().ptr, image.bytes.data(), image.bytes.size());
	auto isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	auto icr = vk::ImageCopy(isrl, {}, isrl, {}, vk::Extent3D{extent, 1});
	auto bic = vk::BufferImageCopy({}, {}, {}, isrl, {}, icr.extent);
	auto cmd = Cmd{m_gfx};
	full_barrier(m_image.get().get().image_view(), m_info.mip_levels, 1, m_layout, vk::ImageLayout::eTransferDstOptimal).transition(cmd.cb);
	cmd.cb.copyBufferToImage(staging.get().buffer, m_image.get().get().image, vk::ImageLayout::eTransferDstOptimal, bic);
	m_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	full_barrier(m_image.get().get().image_view(), m_info.mip_levels, 1, vk::ImageLayout::eTransferDstOptimal, m_layout).transition(cmd.cb);
	if (m_info.mip_levels > 1) { MipMapWriter{m_image.get().get().image, m_image.get().get().extent, cmd.cb, m_info.mip_levels}(); }
}
} // namespace facade
