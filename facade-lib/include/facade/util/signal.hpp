#include <facade/util/delegate.hpp>
#include <memory>

namespace facade {
///
/// \brief RAII observer handle for a shared_ptr<Delegate<Args...>>
///
template <typename... Args>
class Signal {
  public:
	using Id = typename Delegate<Args...>::Id;
	using Fn = typename Delegate<Args...>::Fn;

	Signal() = default;
	///
	/// \brief Attach callback fn to delegate (and store associated Id)
	///
	Signal(std::shared_ptr<Delegate<Args...>> const& delegate, Fn fn) : m_delegate(delegate), m_id(delegate->attach(std::move(fn))) {}

	Signal(Signal&& rhs) = default;
	Signal& operator=(Signal&& rhs) = default;

	~Signal() { reset(); }

	///
	/// \brief Detach associated callback (if attached)
	///
	void reset() {
		if (auto delegate = m_delegate.lock()) {
			delegate->detach(m_id);
			m_delegate = {};
		}
	}

	explicit operator bool() const { return m_delegate.lock() != nullptr; }

  private:
	std::weak_ptr<Delegate<Args...>> m_delegate{};
	Id m_id{};

	friend class Delegate<Args...>;
};
} // namespace facade
