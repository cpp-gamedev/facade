#pragma once
#include <concepts>
#include <iterator>

namespace facade {
template <typename It, typename Index>
class Enumerator {
  public:
	using value_type = decltype(*std::declval<It>());

	struct Entry {
		value_type value;
		Index index;
	};

	class iterator {
	  public:
		using value_type = Entry;
		using difference_type = void;

		constexpr iterator(It it, Index index) : m_it(it), m_index(index) {}

		constexpr value_type operator*() const { return {*m_it, m_index}; }
		constexpr iterator& operator++() { return (++m_it, ++m_index, *this); }

		constexpr bool operator==(iterator const&) const = default;

	  private:
		It m_it{};
		Index m_index{};
	};
	using const_iterator = iterator;

	constexpr Enumerator(It first, It last, Index offset) : m_begin(first), m_end(last), m_offset(offset), m_size(std::distance(m_begin, m_end)) {}

	constexpr iterator begin() const { return {m_begin, m_offset}; }
	constexpr iterator end() const { return {m_end, m_offset + m_size}; }

  private:
	It m_begin;
	It m_end;
	Index m_offset;
	std::size_t m_size;
};

template <typename Container, typename Index = std::size_t>
constexpr auto enumerate(Container&& container, Index offset = {}) {
	return Enumerator<decltype(container.begin()), Index>{container.begin(), container.end(), offset};
}
} // namespace facade
