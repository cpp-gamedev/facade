#pragma once
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace facade {
struct Rgb {
	glm::tvec3<std::uint8_t> channels{0xff, 0xff, 0xff};
	float intensity{1.0f};

	static constexpr float to_f32(std::uint8_t channel) { return static_cast<float>(channel) / static_cast<float>(0xff); }
	static constexpr float to_u8(float normalized) { return static_cast<std::uint8_t>(normalized * static_cast<float>(0xff)); }
	static glm::vec4 to_srgb(glm::vec4 const& linear);
	static glm::vec4 to_linear(glm::vec4 const& srgb);

	static constexpr Rgb make(glm::vec3 const& serialized, float intensity = 1.0f) {
		return {
			.channels = {to_u8(serialized.x), to_u8(serialized.y), to_u8(serialized.z)},
			.intensity = intensity,
		};
	}

	constexpr glm::vec3 to_vec3() const { return intensity * glm::vec3{to_f32(channels.x), to_f32(channels.y), to_f32(channels.z)}; }
	constexpr glm::vec4 to_vec4(float alpha = 1.0f) const { return {to_vec3(), alpha}; }
};
} // namespace facade
