#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <vector>

namespace facade {
///
/// \brief Holds an ordered list of callback Fns, each associated with a unique Id
///
template <typename... Args>
class Delegate {
  public:
	using Id = std::uint64_t;
	using Fn = std::function<void(Args const&...)>;

	///
	/// \brief Attach a callback
	/// \returns Associated Id (use to detach)
	///
	Id attach(Fn fn) {
		if (!fn) { return {}; }
		auto const id = ++m_next_id;
		m_entries.push_back(Entry{.fn = std::move(fn), .id = id});
		return id;
	}

	///
	/// \brief Detach callback associated with id
	///
	void detach(Id id) {
		std::erase_if(m_entries, [id](Entry const& e) { return e.id == id; });
	}

	///
	/// \brief Dispatch all stored callbacks
	///
	void dispatch(Args const&... args) const {
		for (auto const& [fn, _] : m_entries) { fn(args...); }
	}

	void operator()(Args const&... args) const { dispatch(args...); }

  private:
	struct Entry {
		Fn fn{};
		Id id{};
	};

	std::vector<Entry> m_entries{};
	Id m_next_id{};
};
} // namespace facade
