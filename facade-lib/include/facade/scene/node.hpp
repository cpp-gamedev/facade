#pragma once
#include <facade/scene/id.hpp>
#include <facade/scene/transform.hpp>
#include <facade/util/type_id.hpp>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace facade {
template <typename Type>
concept Attachable = std::is_move_constructible_v<Type>;

class Node {
  public:
	Node() = default;
	Node(Node&&) = default;
	Node& operator=(Node&&) = default;

	template <Attachable Type>
	Type& attach(Type t) {
		auto ret = std::make_unique<Model<Type>>(std::move(t));
		auto [it, _] = m_components.insert_or_assign(TypeId::make<Type>(), std::move(ret));
		return static_cast<Model<Type>&>(*it->second).t;
	}

	template <Attachable Type>
	Type* find() const {
		if (auto it = m_components.find(TypeId::make<Type>()); it != m_components.end()) { return &static_cast<Model<Type>&>(*it->second).t; }
		return nullptr;
	}

	template <Attachable Type>
	void detach() {
		m_components.erase(TypeId::make<Type>());
	}

	Id<Node> id() const { return m_id; }

	std::span<Node> children() { return m_children; }
	std::span<Node const> children() const { return m_children; }

	Transform transform{};
	std::vector<Transform> instances{};
	std::string name{};

  private:
	struct Hasher {
		std::size_t operator()(TypeId const& type_id) const { return std::hash<std::size_t>{}(type_id.value()); }
	};

	struct Base {
		virtual ~Base() = default;
	};
	template <typename T>
	struct Model : Base {
		T t;
		Model(T&& t) : t(std::move(t)) {}
	};

	std::unordered_map<TypeId, std::unique_ptr<Base>, Hasher> m_components{};
	std::vector<Node> m_children{};
	Id<Node> m_id{};

	friend class Scene;
};
// } // namespace refactor
} // namespace facade
