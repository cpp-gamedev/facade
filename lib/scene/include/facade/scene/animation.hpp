#pragma once
#include <facade/scene/id.hpp>
#include <facade/scene/interpolator.hpp>
#include <facade/scene/morph_weights.hpp>
#include <numeric>
#include <variant>

namespace facade {
struct Node;

MorphWeights lerp(MorphWeights const& a, MorphWeights const& b, float t);

struct Animator {
	struct Translate : Interpolator<glm::vec3> {};
	struct Rotate : Interpolator<glm::quat> {};
	struct Scale : Interpolator<glm::vec3> {};
	struct Morph : Interpolator<MorphWeights> {};

	std::variant<Translate, Rotate, Scale, Morph> channel{};
	std::optional<Id<Node>> target{};

	float duration() const;
	void update(std::span<Node> nodes, float time) const;
};

class Animation {
  public:
	void add(Animator animator);
	void update(std::span<Node> nodes, float dt);
	bool enabled() const { return time_scale > 0.0f; }

	float duration() const { return m_duration; }

	std::string name{};
	float time_scale{1.0f};
	float elapsed{};

  private:
	std::vector<Animator> m_animators{};
	float m_duration{};
};
} // namespace facade
