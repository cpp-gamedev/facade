#pragma once
#include <glm/gtx/quaternion.hpp>

namespace facade {
inline constexpr auto quat_identity_v = glm::identity<glm::quat>();
inline constexpr auto matrix_identity_v = glm::identity<glm::mat4x4>();

class Transform {
  public:
	struct Data {
		glm::vec3 position{};
		glm::quat orientation{quat_identity_v};
		glm::vec3 scale{1.0f};
	};

	Data const& data() const { return m_data; }
	glm::vec3 position() const { return m_data.position; }
	glm::quat orientation() const { return m_data.orientation; }
	glm::vec3 scale() const { return m_data.scale; }

	Transform& set_data(Data data);
	Transform& set_position(glm::vec3 position);
	Transform& set_orientation(glm::quat orientation);
	Transform& set_scale(glm::vec3 position);

	Transform& rotate(float radians, glm::vec3 const& axis);
	Transform& set_matrix(glm::mat4 mat);

	glm::mat4 const& matrix() const;
	void recompute() const;
	bool is_dirty() const { return m_dirty; }

  private:
	Transform& set_dirty();

	mutable glm::mat4x4 m_matrix{matrix_identity_v};
	Data m_data{};
	mutable bool m_dirty{};
};

// impl

inline Transform& Transform::set_data(Data data) {
	m_data = data;
	return set_dirty();
}

inline Transform& Transform::set_position(glm::vec3 position) {
	m_data.position = position;
	return set_dirty();
}

inline Transform& Transform::set_orientation(glm::quat orientation) {
	m_data.orientation = orientation;
	return set_dirty();
}

inline Transform& Transform::set_scale(glm::vec3 scale) {
	m_data.scale = scale;
	return set_dirty();
}

inline Transform& Transform::rotate(float radians, glm::vec3 const& axis) {
	m_data.orientation = glm::rotate(m_data.orientation, radians, axis);
	return set_dirty();
}

inline glm::mat4 const& Transform::matrix() const {
	if (is_dirty()) { recompute(); }
	return m_matrix;
}

inline void Transform::recompute() const {
	m_matrix = glm::translate(matrix_identity_v, m_data.position) * glm::toMat4(m_data.orientation) * glm::scale(matrix_identity_v, m_data.scale);
	m_dirty = false;
}

inline Transform& Transform::set_dirty() {
	m_dirty = true;
	return *this;
}
} // namespace facade
