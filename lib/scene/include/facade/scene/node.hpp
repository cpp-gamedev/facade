#pragma once
#include <facade/scene/id.hpp>
#include <facade/util/transform.hpp>
#include <facade/util/type_id.hpp>
#include <optional>
#include <unordered_map>
#include <vector>

namespace facade {
struct Node {
	struct Hasher {
		std::size_t operator()(TypeId const& id) const { return std::hash<std::size_t>{}(id.value()); }
	};

	Transform transform{};
	std::vector<Transform> instances{};
	std::unordered_map<TypeId, std::size_t, Hasher> attachments{};
	std::vector<Id<Node>> children{};
	Id<Node> self{};
	std::string name{};

	template <typename T>
	void attach(Id<T> id) {
		attachments.insert_or_assign(TypeId::make<T>(), id);
	}

	template <typename T>
	std::optional<Id<T>> find() const {
		if (auto it = attachments.find(TypeId::make<T>()); it != attachments.end()) { return it->second; }
		return {};
	}

	template <typename T>
	void detach() {
		attachments.erase(TypeId::make<T>());
	}

	template <typename Container>
	static auto find_child(Container&& nodes, std::optional<Id<Node>> child) {
		if (!child || *child >= std::size(nodes)) { return nullptr; }
		return &nodes[*child];
	}
};
} // namespace facade
