#include <facade/scene/animator.hpp>
#include <facade/scene/node.hpp>
#include <facade/util/bool.hpp>
#include <algorithm>

namespace facade {
void Animator::update(Node& out_node, float dt) {
	elapsed += dt;
	auto const duration = std::max({translation.duration(), rotation.duration(), scale.duration()});
	if (auto const p = translation(elapsed)) { out_node.transform.set_position(*p); }
	if (auto const o = rotation(elapsed)) { out_node.transform.set_orientation(*o); }
	if (auto const s = scale(elapsed)) { out_node.transform.set_scale(*s); }
	if (elapsed > duration) { elapsed = {}; }
}
} // namespace facade
