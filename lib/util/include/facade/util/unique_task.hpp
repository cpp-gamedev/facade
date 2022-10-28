#pragma once
#include <memory>

namespace facade {
template <typename T>
class UniqueTask;

///
/// \brief Type erased move-only callable (discount std::move_only_function)
///
template <typename Ret, typename... Args>
class UniqueTask<Ret(Args...)> {
  public:
	UniqueTask() = default;

	template <typename T>
		requires(!std::same_as<UniqueTask, T> && std::is_invocable_r_v<Ret, T, Args...>)
	UniqueTask(T t) : m_func(std::make_unique<Func<T>>(std::move(t))) {}

	Ret operator()(Args... args) const {
		assert(m_func);
		return (*m_func)(args...);
	}

	explicit operator bool() const { return m_func != nullptr; }

  private:
	struct Base {
		virtual ~Base() = default;
		virtual void operator()() = 0;
	};
	template <typename F>
	struct Func : Base {
		F f;
		Func(F&& f) : f(std::move(f)) {}
		void operator()() final { f(); }
	};

	std::unique_ptr<Base> m_func{};
};
} // namespace facade
