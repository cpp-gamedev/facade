#include <bin/shaders.hpp>
#include <bin/default_vert.spv.hpp>
#include <bin/lit_frag.spv.hpp>
#include <bin/skinned_vert.spv.hpp>
#include <bin/skybox_frag.spv.hpp>
#include <bin/unlit_frag.spv.hpp>

namespace facade {
namespace {
template <std::size_t N>
std::span<std::byte const> to_bytes(unsigned char const (&arr)[N]) {
	return {reinterpret_cast<std::byte const*>(arr), N};
}
} // namespace

Shader vert::default_() {
	return {
		.id = "default.vert",
		.spir_v = SpirV::View::from_bytes(to_bytes(default_vert_v)),
	};
}

Shader vert::skinned() {
	return {
		.id = "skinned.vert",
		.spir_v = SpirV::View::from_bytes(to_bytes(skinned_vert_v)),
	};
}

Shader frag::unlit() {
	return {
		.id = "unlit.frag",
		.spir_v = SpirV::View::from_bytes(to_bytes(unlit_frag_v)),
	};
}

Shader frag::lit() {
	return {
		.id = "lit.frag",
		.spir_v = SpirV::View::from_bytes(to_bytes(lit_frag_v)),
	};
}

Shader frag::skybox() {
	return {
		.id = "skybox.frag",
		.spir_v = SpirV::View::from_bytes(to_bytes(skybox_frag_v)),
	};
}
} // namespace facade
