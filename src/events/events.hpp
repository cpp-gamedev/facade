#pragma once
#include <facade/util/type_id.hpp>
#include <facade/util/unique_task.hpp>
#include <algorithm>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace facade {
template <typename EventT>
using OnEvent = UniqueTask<void(EventT const&)>;

class Events {
  public:
	using Id = std::uint64_t;

	template <typename EventT>
	Id attach(OnEvent<EventT> callback) {
		if (!callback) { return {}; }
		return replace_or_attach({}, std::move(callback));
	}

	template <typename EventT>
	void detach(Id const id) {
		if (id == Id{}) { return; }
		auto lock = std::scoped_lock{m_mutex};
		auto const it = m_map.find(TypeId::make<EventT>());
		if (it == m_map.end()) { return; }
		auto& model = static_cast<Model<EventT>&>(*it->second);
		std::erase_if(model.entries, [id](Entry<EventT> const& e) { return e.id == id; });
	}

	template <typename EventT>
	Id replace_or_attach(Id id, OnEvent<EventT> callback) {
		if (!callback) {
			detach<EventT>(id);
			return {};
		}
		auto lock = std::scoped_lock{m_mutex};
		auto& model = get_or_make<EventT>(lock);
		if (id != Id{}) {
			auto const cmp = [id](Entry<EventT> const& e) { return e.id == id; };
			if (auto it = std::find_if(model.entries.begin(), model.entries.end(), cmp); it != model.entries.end()) {
				it->callback = std::move(callback);
				return id;
			}
		}
		id = ++model.next_id;
		model.entries.push_back({std::move(callback), id});
		return id;
	}

	template <typename EventT>
	void dispatch(EventT const& event) const {
		auto lock = std::scoped_lock{m_mutex};
		auto const it = m_map.find(TypeId::make<EventT>());
		if (it == m_map.end()) { return; }
		auto const& model = static_cast<Model<EventT>&>(*it->second);
		for (auto const& [callback, _] : model.entries) { callback(event); }
	}

  private:
	template <typename EventT>
	struct Entry {
		OnEvent<EventT> callback{};
		Id id{};
	};

	struct Base {
		virtual ~Base() = default;
	};
	template <typename EventT>
	struct Model : Base {
		std::vector<Entry<EventT>> entries{};
		Id next_id{};
	};

	struct Hasher {
		std::size_t operator()(TypeId const& id) const { return std::hash<std::size_t>{}(id.value()); }
	};

	using Lock = std::scoped_lock<std::mutex>;

	template <typename EventT>
	Model<EventT>& get_or_make(Lock const&) {
		auto const id = TypeId::make<EventT>();
		auto const it = m_map.find(id);
		if (it != m_map.end()) { return static_cast<Model<EventT>&>(*it->second); }
		auto model = std::make_unique<Model<EventT>>();
		auto& ret = *model;
		m_map.insert_or_assign(id, std::move(model));
		return ret;
	}

	std::unordered_map<TypeId, std::unique_ptr<Base>, Hasher> m_map{};
	mutable std::mutex m_mutex{};
};

template <typename EventT>
class Observer {
  public:
	Observer() = default;

	Observer(std::shared_ptr<Events> const& events, OnEvent<EventT> callback = {}) : m_events(events), m_id(events->attach(std::move(callback))) {}
	Observer(Observer&& rhs) noexcept : Observer() { swap(rhs); }
	Observer& operator=(Observer rhs) noexcept { return (swap(rhs), *this); }
	~Observer() { detach(); }

	void attach(OnEvent<EventT> callback) {
		if (auto events = m_events.lock()) { m_id = events->replace_or_attach(m_id, std::move(callback)); }
	}

	void detach() {
		if (m_id == Events::Id{}) { return; }
		if (auto events = m_events.lock()) { events->detach<EventT>(m_id); }
		m_id = {};
	}

	explicit operator bool() const { return m_id != Events::Id{} && m_events.lock() != nullptr; }

	void swap(Observer& rhs) noexcept {
		std::swap(m_events, rhs.m_events);
		std::swap(m_id, rhs.m_id);
	}

  private:
	std::weak_ptr<Events> m_events{};
	Events::Id m_id{};
};
} // namespace facade
