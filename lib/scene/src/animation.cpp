#include <facade/scene/animation.hpp>
#include <facade/scene/node.hpp>
#include <facade/util/bool.hpp>
#include <facade/util/zip_ranges.hpp>

namespace facade {
MorphWeights lerp(MorphWeights const& a, MorphWeights const& b, float t) {
	assert(a.weights.size() == b.weights.size());
	auto ret = MorphWeights{};
	for (auto [p, q] : zip_ranges(a.weights.span(), b.weights.span())) { ret.weights.insert(std::lerp(p, q, t)); }
	return ret;
}

void Animator::update(std::span<Node> nodes, float time) const {
	if (translation.target) {
		assert(*translation.target < nodes.size());
		if (auto const p = translation.interpolate(time)) { nodes[*translation.target].transform.set_position(*p); }
	}
	if (rotation.target) {
		assert(*rotation.target < nodes.size());
		if (auto const o = rotation.interpolate(time)) { nodes[*rotation.target].transform.set_orientation(*o); }
	}
	if (scale.target) {
		assert(*scale.target < nodes.size());
		if (auto const s = scale.interpolate(time)) { nodes[*scale.target].transform.set_scale(*s); }
	}
	if (weights.target) {
		assert(*weights.target < nodes.size());
		if (auto const w = weights.interpolate(time)) { nodes[*weights.target].weights = *w; }
	}
}

void Animation::update(std::span<Node> nodes, float dt) {
	elapsed += dt;
	animator.update(nodes, elapsed);
	if (elapsed > animator.duration()) { elapsed = {}; }
}
} // namespace facade
