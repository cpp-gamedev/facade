#pragma once
#include <facade/util/logger.hpp>
#include <facade/util/transform.hpp>
#include <optional>

namespace facade {
class Node;

template <typename T>
struct Timeline {
	struct Keyframe {
		T value{};
		float timestamp{};
	};

	std::vector<Keyframe> keyframes{};
	float duration{};

	std::optional<std::size_t> index_for(float time) const {
		if (duration <= 0.0f || keyframes.empty()) { return {}; }

		auto const it = std::lower_bound(keyframes.begin(), keyframes.end(), time, [](Keyframe const& k, float time) { return k.timestamp < time; });
		if (it == keyframes.end()) { return {}; }
		return static_cast<std::size_t>(it - keyframes.begin());
	}
};

constexpr glm::vec3 lerp(glm::vec3 const& a, glm::vec3 const& b, float const t) {
	return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t)};
}

inline glm::quat lerp(glm::quat const& a, glm::quat const& b, float const t) { return glm::slerp(a, b, t); }

template <typename T>
struct Interpolator {
	Timeline<T> timeline{};

	std::optional<T> current{};
	float elapsed{};

	void tick(float dt) {
		if (timeline.keyframes.empty()) { return; }

		elapsed += dt;
		while (elapsed >= timeline.duration) { elapsed -= timeline.duration; }

		auto const i_next = timeline.index_for(elapsed);
		if (!i_next) { return; }

		assert(*i_next < timeline.keyframes.size());
		auto const& next = timeline.keyframes[*i_next];
		assert(elapsed < next.timestamp);
		if (*i_next == 0) {
			current = next.value;
			return;
		}

		auto const& prev = timeline.keyframes[*i_next - 1];
		assert(prev.timestamp < elapsed);
		auto const t = (elapsed - prev.timestamp) / (next.timestamp - prev.timestamp);
		using facade::lerp;
		using std::lerp;
		current = lerp(prev.value, next.value, t);
	}
};

template <>
struct Interpolator<Transform> {
	Interpolator<glm::vec3> translation{};
	Interpolator<glm::quat> rotation{};
	Interpolator<glm::vec3> scale{};

	void update(float dt, Transform& out) {
		translation.tick(dt);
		rotation.tick(dt);
		scale.tick(dt);
		if (translation.current) { out.set_position(*translation.current); }
		if (rotation.current) { out.set_orientation(*rotation.current); }
		if (scale.current) { out.set_scale(*scale.current); }
	}
};
} // namespace facade
