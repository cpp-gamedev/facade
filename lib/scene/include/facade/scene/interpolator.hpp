#pragma once
#include <facade/util/logger.hpp>
#include <facade/util/transform.hpp>
#include <optional>

namespace facade {
constexpr glm::vec3 lerp(glm::vec3 const& a, glm::vec3 const& b, float const t) {
	return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t)};
}

inline glm::quat lerp(glm::quat const& a, glm::quat const& b, float const t) { return glm::slerp(a, b, t); }

enum class Interpolation {
	eLinear,
	eStep,
	eCubic,
};

template <typename T>
struct Interpolator {
	struct Keyframe {
		T value{};
		float timestamp{};
	};

	std::vector<Keyframe> keyframes{};
	Interpolation interpolation{};

	float duration() const { return keyframes.empty() ? 0.0f : keyframes.back().timestamp; }

	std::optional<std::size_t> index_for(float time) const {
		if (keyframes.empty()) { return {}; }

		auto const it = std::lower_bound(keyframes.begin(), keyframes.end(), time, [](Keyframe const& k, float time) { return k.timestamp < time; });
		if (it == keyframes.end()) { return {}; }
		return static_cast<std::size_t>(it - keyframes.begin());
	}

	std::optional<T> operator()(float elapsed) const {
		if (keyframes.empty()) { return {}; }

		auto const i_next = index_for(elapsed);
		if (!i_next) { return keyframes.back().value; }

		assert(*i_next < keyframes.size());
		auto const& next = keyframes[*i_next];
		assert(elapsed <= next.timestamp);
		if (*i_next == 0) { return next.value; }

		auto const& prev = keyframes[*i_next - 1];
		assert(prev.timestamp < elapsed);
		if (interpolation == Interpolation::eStep) { return prev.value; }

		auto const t = (elapsed - prev.timestamp) / (next.timestamp - prev.timestamp);
		using facade::lerp;
		using std::lerp;
		return lerp(prev.value, next.value, t);
	}
};
} // namespace facade
