#pragma once
#include <facade/util/colour_space.hpp>
#include <facade/util/image.hpp>
#include <facade/vk/defer.hpp>
#include <facade/vk/gfx.hpp>
#include <span>

namespace facade {
struct SamplerCreateInfo {
	std::string name{"(unnamed)"};
	vk::SamplerAddressMode mode_s{vk::SamplerAddressMode::eRepeat};
	vk::SamplerAddressMode mode_t{vk::SamplerAddressMode::eRepeat};
	vk::Filter min{vk::Filter::eLinear};
	vk::Filter mag{vk::Filter::eLinear};
};

struct TextureCreateInfo {
	std::string name{"(unnamed)"};
	bool mip_mapped{true};
	ColourSpace colour_space{ColourSpace::eSrgb};
};

class Sampler {
  public:
	using CreateInfo = SamplerCreateInfo;

	explicit Sampler(Gfx const& gfx, CreateInfo info = {});

	std::string_view name() const { return m_name; }
	vk::Sampler sampler() const { return *m_sampler.get(); }

  private:
	std::string m_name{};
	Defer<vk::UniqueSampler> m_sampler{};
};

class Texture {
  public:
	using CreateInfo = TextureCreateInfo;

	static std::uint32_t mip_levels(vk::Extent2D extent);

	Texture(Gfx const& gfx, vk::Sampler sampler, Image::View image, CreateInfo info = {});

	std::string_view name() const { return m_name; }
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
	std::string m_name{};
};
} // namespace facade
