#include <facade/util/flex_array.hpp>
#include <facade/util/geometry.hpp>
#include <facade/util/nvec3.hpp>
#include <array>

auto facade::make_cube(glm::vec3 size, glm::vec3 rgb, glm::vec3 const o) -> Geometry {
	auto ret = Geometry{};
	auto const h = 0.5f * size;
	ret.vertices = {
		// front
		{{o.x - h.x, o.y + h.y, o.z + h.z}, rgb, front_v, {0.0f, 0.0f}},
		{{o.x + h.x, o.y + h.y, o.z + h.z}, rgb, front_v, {1.0f, 0.0f}},
		{{o.x + h.x, o.y - h.y, o.z + h.z}, rgb, front_v, {1.0f, 1.0f}},
		{{o.x - h.x, o.y - h.y, o.z + h.z}, rgb, front_v, {0.0f, 1.0f}},

		// back
		{{o.x + h.x, o.y + h.y, o.z - h.z}, rgb, -front_v, {0.0f, 0.0f}},
		{{o.x - h.x, o.y + h.y, o.z - h.z}, rgb, -front_v, {1.0f, 0.0f}},
		{{o.x - h.x, o.y - h.y, o.z - h.z}, rgb, -front_v, {1.0f, 1.0f}},
		{{o.x + h.x, o.y - h.y, o.z - h.z}, rgb, -front_v, {0.0f, 1.0f}},

		// right
		{{o.x + h.x, o.y + h.y, o.z + h.z}, rgb, right_v, {0.0f, 0.0f}},
		{{o.x + h.x, o.y + h.y, o.z - h.z}, rgb, right_v, {1.0f, 0.0f}},
		{{o.x + h.x, o.y - h.y, o.z - h.z}, rgb, right_v, {1.0f, 1.0f}},
		{{o.x + h.x, o.y - h.y, o.z + h.z}, rgb, right_v, {0.0f, 1.0f}},

		// left
		{{o.x - h.x, o.y + h.y, o.z - h.z}, rgb, -right_v, {0.0f, 0.0f}},
		{{o.x - h.x, o.y + h.y, o.z + h.z}, rgb, -right_v, {1.0f, 0.0f}},
		{{o.x - h.x, o.y - h.y, o.z + h.z}, rgb, -right_v, {1.0f, 1.0f}},
		{{o.x - h.x, o.y - h.y, o.z - h.z}, rgb, -right_v, {0.0f, 1.0f}},

		// top
		{{o.x - h.x, o.y + h.y, o.z - h.z}, rgb, up_v, {0.0f, 0.0f}},
		{{o.x + h.x, o.y + h.y, o.z - h.z}, rgb, up_v, {1.0f, 0.0f}},
		{{o.x + h.x, o.y + h.y, o.z + h.z}, rgb, up_v, {1.0f, 1.0f}},
		{{o.x - h.x, o.y + h.y, o.z + h.z}, rgb, up_v, {0.0f, 1.0f}},

		// bottom
		{{o.x - h.x, o.y - h.y, o.z + h.z}, rgb, -up_v, {0.0f, 0.0f}},
		{{o.x + h.x, o.y - h.y, o.z + h.z}, rgb, -up_v, {1.0f, 0.0f}},
		{{o.x + h.x, o.y - h.y, o.z - h.z}, rgb, -up_v, {1.0f, 1.0f}},
		{{o.x - h.x, o.y - h.y, o.z - h.z}, rgb, -up_v, {0.0f, 1.0f}},
	};
	// clang-format off
	ret.indices = {
		 0,  1,  2,  2,  3,  0,
		 4,  5,  6,  6,  7,  4,
		 8,  9, 10, 10, 11,  8,
		12, 13, 14, 14, 15, 12,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20,
	};
	// clang-format on
	return ret;
}

auto facade::make_cubed_sphere(float diam, std::uint32_t quads_per_side, glm::vec3 rgb) -> Geometry {
	Geometry ret;
	auto quad_count = static_cast<std::uint32_t>(quads_per_side * quads_per_side);
	ret.vertices.reserve(quad_count * 4 * 6);
	ret.indices.reserve(quad_count * 6 * 6);
	auto const bl = glm::vec3{-1.0f, -1.0f, 1.0f};
	auto points = std::vector<std::pair<glm::vec3, glm::vec2>>{};
	points.reserve(4 * quad_count);
	auto const s = 2.0f / static_cast<float>(quads_per_side);
	auto const duv = 1.0f / static_cast<float>(quads_per_side);
	float u = 0.0f;
	float v = 0.0f;
	for (std::uint32_t row = 0; row < quads_per_side; ++row) {
		v = static_cast<float>(row) * duv;
		for (std::uint32_t col = 0; col < quads_per_side; ++col) {
			u = static_cast<float>(col) * duv;
			auto const o = s * glm::vec3{static_cast<float>(col), static_cast<float>(row), 0.0f};
			points.push_back({glm::vec3(bl + o), glm::vec2{u, 1.0f - v}});
			points.push_back({glm::vec3(bl + glm::vec3{s, 0.0f, 0.0f} + o), glm::vec2{u + duv, 1.0 - v}});
			points.push_back({glm::vec3(bl + glm::vec3{s, s, 0.0f} + o), glm::vec2{u + duv, 1.0f - v - duv}});
			points.push_back({glm::vec3(bl + glm::vec3{0.0f, s, 0.0f} + o), glm::vec2{u, 1.0f - v - duv}});
		}
	}
	auto add_side = [rgb, diam, &ret](std::vector<std::pair<glm::vec3, glm::vec2>>& out_points, nvec3 (*transform)(glm::vec3 const&)) {
		auto idx = std::uint32_t{};
		auto indices = FlexArray<std::uint32_t, 4>{};
		auto update_indices = [&] {
			if (indices.size() == 4) {
				auto const span = indices.span();
				std::array const arr = {span[0], span[1], span[2], span[2], span[3], span[0]};
				std::copy(arr.begin(), arr.end(), std::back_inserter(ret.indices));
				indices.clear();
			}
		};
		for (auto const& p : out_points) {
			update_indices();
			++idx;
			auto const pt = transform(p.first).value() * diam * 0.5f;
			indices.insert(static_cast<std::uint32_t>(ret.vertices.size()));
			ret.vertices.push_back({pt, rgb, pt, p.second});
		}
		update_indices();
	};
	add_side(points, [](glm::vec3 const& p) { return nvec3(p); });
	add_side(points, [](glm::vec3 const& p) { return nvec3(glm::rotate(p, glm::radians(180.0f), up_v)); });
	add_side(points, [](glm::vec3 const& p) { return nvec3(glm::rotate(p, glm::radians(90.0f), up_v)); });
	add_side(points, [](glm::vec3 const& p) { return nvec3(glm::rotate(p, glm::radians(-90.0f), up_v)); });
	add_side(points, [](glm::vec3 const& p) { return nvec3(glm::rotate(p, glm::radians(90.0f), right_v)); });
	add_side(points, [](glm::vec3 const& p) { return nvec3(glm::rotate(p, glm::radians(-90.0f), right_v)); });
	return ret;
}
