#pragma once
#include <facade/scene/transform.hpp>
#include <facade/util/nvec3.hpp>

namespace facade {
class FlyCam {
  public:
	float look_speed{0.001f};
	float move_speed{10.0f};

	FlyCam(Transform& out_target) : m_target(out_target), m_front(m_target.orientation() * front_v), m_right(glm::cross(up_v, m_front.value())) {}
	~FlyCam() { apply(); }

	void move_right(float dx) { m_dxyz += move_speed * dx * m_right.value(); }
	void move_front(float dz) { m_dxyz += move_speed * dz * m_front.value(); }
	void move_up(float dy) { m_dxyz.y += move_speed * dy; }

	void rotate(glm::vec2 yaw_pitch) {
		m_front.rotate(look_speed * yaw_pitch.x, -up_v);
		m_front.rotate(look_speed * yaw_pitch.y, m_right);
		m_right = glm::cross(up_v, m_front.value());
	}

	void apply() const {
		auto data = m_target.data();
		data.position += m_dxyz;
		auto const mat = glm::mat3x3{m_right, glm::cross(m_front.value(), m_right.value()), m_front};
		data.orientation = glm::quat_cast(mat);
		m_target.set_data(data);
	}

	explicit operator bool() const { return true; }

  private:
	Transform& m_target;
	glm::vec3 m_dxyz{};
	nvec3 m_front{};
	nvec3 m_right{};
};
} // namespace facade
