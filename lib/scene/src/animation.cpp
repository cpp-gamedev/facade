#include <facade/scene/animation.hpp>
#include <facade/scene/node.hpp>
#include <facade/util/bool.hpp>
#include <facade/util/visitor.hpp>
#include <facade/util/zip_ranges.hpp>

namespace facade {
MorphWeights lerp(MorphWeights const& a, MorphWeights const& b, float t) {
	assert(a.weights.size() == b.weights.size());
	auto ret = MorphWeights{};
	for (auto [p, q] : zip_ranges(a.weights.span(), b.weights.span())) { ret.weights.insert(std::lerp(p, q, t)); }
	return ret;
}

float Animator::duration() const {
	return std::visit([](auto const& ch) { return ch.duration(); }, channel);
}

void Animator::update(std::span<Node> nodes, float time) const {
	if (!target) { return; }
	assert(*target < nodes.size());
	auto& node = nodes[*target];
	auto const visitor = Visitor{
		[time, &node](Translate const& translate) {
			if (auto const p = translate(time)) { node.transform.set_position(*p); }
		},
		[time, &node](Rotate const& rotate) {
			if (auto const o = rotate(time)) { node.transform.set_orientation(*o); }
		},
		[time, &node](Scale const& scale) {
			if (auto const s = scale(time)) { node.transform.set_scale(*s); }
		},
		[time, &node](Morph const& morph) {
			if (auto const m = morph(time)) { node.weights = *m; }
		},
	};
	std::visit(visitor, channel);
}

void Animation::add(Animator animator) {
	m_animators.push_back(std::move(animator));
	for (auto& animator : m_animators) { m_duration = std::max(animator.duration(), m_duration); }
}

void Animation::update(std::span<Node> nodes, float dt) {
	if (!enabled()) { return; }
	elapsed += dt * time_scale;
	for (auto& animator : m_animators) { animator.update(nodes, elapsed); }
	if (elapsed > m_duration) { elapsed = {}; }
}
} // namespace facade
