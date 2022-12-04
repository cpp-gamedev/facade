#pragma once
#include <facade/scene/id.hpp>
#include <facade/scene/interpolator.hpp>
#include <facade/scene/morph_weights.hpp>
#include <numeric>

namespace facade {
struct Node;

MorphWeights lerp(MorphWeights const& a, MorphWeights const& b, float t);

template <typename T>
struct AnimationChannel {
	Interpolator<T> interpolator{};
	std::optional<Id<Node>> target{};

	auto interpolate(float time) const { return interpolator(time); }
};

struct Animator {
	AnimationChannel<glm::vec3> translation{};
	AnimationChannel<glm::quat> rotation{};
	AnimationChannel<glm::vec3> scale{};
	AnimationChannel<MorphWeights> weights{};

	template <typename... U>
	static float max_duration(U&&... channels) {
		return std::max({channels.interpolator.duration()...});
	}

	float duration() const { return max_duration(translation, rotation, scale, weights); }
	void update(std::span<Node> nodes, float time) const;
};

struct Animation {
	std::vector<Animator> animators{};
	float elapsed{};

	void update(std::span<Node> nodes, float dt);
};
} // namespace facade
