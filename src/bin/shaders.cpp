#include <bin/shaders.hpp>
#include <bin/default_vert.spv.hpp>
#include <bin/lit_frag.spv.hpp>
#include <bin/skybox_frag.spv.hpp>
#include <bin/unlit_frag.spv.hpp>

namespace facade {
namespace {
template <std::size_t N>
std::span<std::byte const> to_bytes(unsigned char const (&arr)[N]) {
	return {reinterpret_cast<std::byte const*>(arr), N};
}
} // namespace

Shader shaders::unlit() {
	return {
		.id = "unlit",
		.vert = SpirV::View::from_bytes(to_bytes(default_vert_v)),
		.frag = SpirV::View::from_bytes(to_bytes(unlit_frag_v)),
	};
}

Shader shaders::lit() {
	return {
		.id = "lit",
		.vert = SpirV::View::from_bytes(to_bytes(default_vert_v)),
		.frag = SpirV::View::from_bytes(to_bytes(lit_frag_v)),
	};
}

Shader shaders::skybox() {
	return {
		.id = "skybox",
		.vert = SpirV::View::from_bytes(to_bytes(default_vert_v)),
		.frag = SpirV::View::from_bytes(to_bytes(skybox_frag_v)),
	};
}
} // namespace facade
