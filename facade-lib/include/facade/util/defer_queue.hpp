#pragma once
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace facade {
class DeferQueue {
  public:
	DeferQueue() = default;
	DeferQueue& operator=(DeferQueue&&) = delete;

	template <typename T>
	void push(T t) {
		auto lock = std::scoped_lock{m_mutex};
		m_stack.push(std::move(t));
	}

	void next() {
		auto lock = std::scoped_lock{m_mutex};
		m_stack.next();
	}

  private:
	struct Erased {
		virtual ~Erased() = default;
	};
	template <typename T>
	struct Model : Erased {
		T t;
		Model(T&& t) : t{std::move(t)} {};
	};

	template <std::size_t Size = 3>
	struct Stack {
		using Row = std::vector<std::unique_ptr<Erased>>;

		Row rows[Size]{};
		std::size_t index{};

		template <typename T>
		void push(T t) {
			rows[index].push_back(std::make_unique<Model<T>>(std::move(t)));
		}

		void next() {
			index = (index + 1) % Size;
			rows[index].clear();
		}
	};

	Stack<> m_stack{};
	std::mutex m_mutex{};
};
} // namespace facade
