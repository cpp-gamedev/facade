#pragma once
#include <facade/util/colour_space.hpp>
#include <facade/util/image.hpp>
#include <facade/vk/defer.hpp>
#include <facade/vk/gfx.hpp>
#include <span>

namespace facade {
class Sampler {
  public:
	struct CreateInfo {
		Gfx gfx{};
		vk::SamplerAddressMode mode_s{vk::SamplerAddressMode::eRepeat};
		vk::SamplerAddressMode mode_t{vk::SamplerAddressMode::eRepeat};
		vk::Filter min{vk::Filter::eLinear};
		vk::Filter mag{vk::Filter::eLinear};
	};

	explicit Sampler(CreateInfo const& info);

	vk::Sampler sampler() const { return *m_sampler.get(); }

  private:
	Defer<vk::UniqueSampler> m_sampler{};
};

class Texture {
  public:
	struct CreateInfo {
		Gfx gfx{};
		vk::Sampler sampler{};

		bool mip_mapped{true};
		ColourSpace colour_space{ColourSpace::eSrgb};
	};

	static std::uint32_t mip_levels(vk::Extent2D extent);
	static vk::UniqueSampler make_sampler(Gfx const& gfx, vk::SamplerAddressMode mode, vk::Filter filter = vk::Filter::eLinear);

	Texture(CreateInfo const& info, Image::View image);

	ImageView view() const { return m_image.get().get().image_view(); }
	DescriptorImage descriptor_image() const { return {*m_image.get().get().view, sampler}; }
	std::uint32_t mip_levels() const { return m_info.mip_levels; }
	ColourSpace colour_space() const { return is_linear(m_info.format) ? ColourSpace::eLinear : ColourSpace::eSrgb; }

	vk::Sampler sampler{};

  private:
	Defer<UniqueImage> m_image{};
	Gfx m_gfx{};
	ImageCreateInfo m_info{};
	vk::ImageLayout m_layout{vk::ImageLayout::eUndefined};
};
} // namespace facade
