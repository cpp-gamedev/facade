#pragma once
#include <facade/util/nvec3.hpp>
#include <facade/util/transform.hpp>

namespace facade {
///
/// \brief Controls for manipulating a Transform as a fly cam.
///
/// Intended to be used as a transient object every frame: destructor applies the accumulated changes to the target Transform.
///
class FlyCam {
  public:
	///
	/// \brief Rotation speed.
	///
	float look_speed{0.001f};
	///
	/// \brief Translation speed.
	///
	float move_speed{10.0f};

	///
	/// \brief Construct a FlyCam targetting a Transform.
	///
	FlyCam(Transform& out_target) : m_target(out_target), m_front(m_target.orientation() * front_v), m_right(glm::cross(up_v, m_front.value())) {}
	~FlyCam() { apply(); }

	///
	/// \brief Translate local right by dx.
	///
	void move_right(float dx) { m_dxyz += move_speed * dx * m_right.value(); }
	///
	/// \brief Translate local front by dz.
	///
	void move_front(float dz) { m_dxyz += move_speed * dz * m_front.value(); }
	///
	/// \brief Translate up by dy.
	///
	/// FlyCam local up always matches global up.
	///
	void move_up(float dy) { m_dxyz.y += move_speed * dy; }

	///
	/// \brief Affect rotation by yaw and pitch.
	///
	void rotate(glm::vec2 yaw_pitch) {
		m_front.rotate(look_speed * yaw_pitch.x, -up_v);
		m_front.rotate(look_speed * yaw_pitch.y, m_right);
		m_right = glm::cross(up_v, m_front.value());
	}

	///
	/// \brief Apply accumulated changes to target Transform.
	///
	void apply() const {
		auto data = m_target.data();
		data.position += m_dxyz;
		auto const mat = glm::mat3x3{m_right, glm::cross(m_front.value(), m_right.value()), m_front};
		data.orientation = glm::quat_cast(mat);
		m_target.set_data(data);
	}

	///
	/// \brief Convenience wrapper for use in if (auto cam = FlyCam{transform}) expressions
	///
	explicit operator bool() const { return true; }

  private:
	Transform& m_target;
	glm::vec3 m_dxyz{};
	nvec3 m_front{};
	nvec3 m_right{};
};
} // namespace facade
