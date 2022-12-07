#include <facade/util/rgb.hpp>
#include <facade/vk/cmd.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/skybox.hpp>
#include <numeric>

namespace facade {
namespace {
template <typename... T>
constexpr void set_rgbs(std::span<Vertex> vertices, glm::vec3 rgb, T... indices) {
	((vertices[static_cast<std::size_t>(indices)].rgb = rgb), ...);
}

constexpr glm::vec4 gradient_v[2] = {
	{0.9254f, 0.9215f, 0.8942f, 1.0f},
	{0.7254f, 0.7215f, 0.6942f, 1.0f},
};

Geometry make_gradient_cubemap_cube() {
	auto ret = make_cube(glm::vec3{1.0f});
	set_rgbs(ret.vertices, Rgb::to_linear(gradient_v[0]), 0, 1, 4, 5, 8, 9, 12, 13, 16, 17, 18, 19);
	set_rgbs(ret.vertices, Rgb::to_linear(gradient_v[1]), 2, 3, 6, 7, 10, 11, 14, 15, 20, 21, 22, 23);
	return ret;
}

std::array<Image::View, 6> make_cubemap_images() {
	static constexpr std::byte black_v[] = {0xff_B, 0xff_B, 0xff_B, 0xff_B};
	static constexpr auto image_v = Image::View{.bytes = black_v, .extent = {1, 1}};
	return {image_v, image_v, image_v, image_v, image_v, image_v};
}
} // namespace

Skybox::Skybox(Gfx const& gfx)
	: m_sampler(gfx), m_cube(gfx, make_gradient_cubemap_cube(), {}, "skybox"), m_cubemap(gfx, m_sampler.sampler(), make_cubemap_images(), "skybox"),
	  m_gfx(gfx) {}

void Skybox::set(std::span<Image::View const> images) { m_cubemap = {m_gfx, m_sampler.sampler(), images, "skybox"}; }

void Skybox::reset() { m_cubemap = {m_gfx, m_sampler.sampler(), make_cubemap_images()}; }
} // namespace facade
