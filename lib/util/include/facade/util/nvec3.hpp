#pragma once
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/vec3.hpp>

namespace facade {
inline constexpr auto right_v = glm::vec3{1.0f, 0.0f, 0.0f};
inline constexpr auto up_v = glm::vec3{0.0f, 1.0f, 0.0f};
inline constexpr auto front_v = glm::vec3{0.0f, 0.0f, 1.0f};

class nvec3 {
  public:
	nvec3() = default;
	nvec3(glm::vec3 dir) : m_value{glm::normalize(dir)} {}

	constexpr glm::vec3 const& value() const { return m_value; }
	constexpr operator glm::vec3 const&() const { return value(); }

	nvec3& rotate(float rad, glm::vec3 axis) {
		m_value = glm::normalize(glm::rotate(m_value, rad, axis));
		return *this;
	}

	nvec3 rotated(float rad, glm::vec3 axis) const {
		auto ret = *this;
		ret.rotate(rad, axis);
		return ret;
	}

  private:
	glm::vec3 m_value{front_v};
};
} // namespace facade
