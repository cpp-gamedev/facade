#pragma once
#include <facade/scene/id.hpp>
#include <facade/util/ptr.hpp>
#include <span>
#include <utility>
#include <vector>

namespace facade {
///
/// \brief Wrapper over an array of resources, each identified by the index as its Id.
///
/// Only Scene has access to the underlying vector (via friend). Other types can operate
/// on const / non-const spans, but cannot modify the array itself.
///
template <typename T>
class ResourceArray {
  public:
	///
	/// \brief Obtain an immutable view into the underlying array.
	/// \returns Immutable span
	///
	std::span<T const> view() const { return m_array; }
	///
	/// \brief Obtain a mutable view into the underlying array.
	/// \returns Mutable span
	///
	std::span<T> view() { return m_array; }

	///
	/// \brief Obtain an immutable pointer to the element at index id.
	/// \param id Id (index) of element
	/// \returns nullptr if id is out of bounds
	///
	Ptr<T const> find(Id<T> id) const {
		if (id >= m_array.size()) { return {}; }
		return &m_array[id];
	}

	///
	/// \brief Obtain a mutable pointer to the element at index id.
	/// \param id Id (index) of element
	/// \returns nullptr if id is out of bounds
	///
	Ptr<T> find(Id<T> index) { return const_cast<Ptr<T>>(std::as_const(*this).find(index)); }

	///
	/// \brief Obtain an immutable reference to the element at index id.
	/// \param id Id (index) of element
	/// \returns const reference to element
	///
	/// id must be valid / in range
	///
	T const& operator[](Id<T> id) const {
		auto ret = find(id);
		assert(ret);
		return *ret;
	}

	///
	/// \brief Obtain an immutable reference to the element at index id.
	/// \param id Id (index) of element
	/// \returns const reference to element
	///
	/// id must be valid / in range
	///
	T& operator[](Id<T> index) { return const_cast<T&>(std::as_const(*this).operator[](index)); }

	///
	/// \brief Obtain the size of the underlying array.
	///
	constexpr std::size_t size() const { return m_array.size(); }
	///
	/// \brief Check if the underlying array is empty.
	///
	constexpr bool empty() const { return m_array.empty(); }

  private:
	std::vector<T> m_array{};

	friend class Scene;
};
} // namespace facade
