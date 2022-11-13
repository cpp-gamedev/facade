#pragma once
#include <facade/scene/id.hpp>
#include <facade/util/transform.hpp>
#include <facade/util/type_id.hpp>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace facade {
///
/// \brief Concept for components attachable to a Node
///
template <typename Type>
concept Attachable = std::is_move_constructible_v<Type>;

///
/// \brief Represents a node in the scene graph.
///
/// Each Node instance contains:
/// - Transform
/// - Attachments
/// - Child nodes
///
class Node {
  public:
	Node() = default;
	Node(Node&&) = default;
	Node& operator=(Node&&) = default;

	///
	/// \brief Attach t to the Node.
	/// \param t object to attach
	/// \returns Reference to attached object
	///
	/// Replaces existing Type, if already attached.
	///
	template <Attachable Type>
	Type& attach(Type t) {
		auto ret = std::make_unique<Model<Type>>(std::move(t));
		auto [it, _] = m_attachments.insert_or_assign(TypeId::make<Type>(), std::move(ret));
		return static_cast<Model<Type>&>(*it->second).t;
	}

	///
	/// \brief Obtain a pointer to attached Type, if present.
	/// \returns nullptr if Type isn't attached
	///
	template <Attachable Type>
	Type* find() const {
		if (auto it = m_attachments.find(TypeId::make<Type>()); it != m_attachments.end()) { return &static_cast<Model<Type>&>(*it->second).t; }
		return nullptr;
	}

	///
	/// \brief Detach Type, if attached.
	///
	template <Attachable Type>
	void detach() {
		m_attachments.erase(TypeId::make<Type>());
	}

	std::size_t attachment_count() const { return m_attachments.size(); }

	///
	/// \brief Obtain the Id of the node
	///
	Id<Node> id() const { return m_id; }

	///
	/// \brief Obtain a mutable view into the child nodes.
	///
	std::span<Node> children() { return m_children; }
	///
	/// \brief Obtain an immutable view into the child nodes.
	///
	std::span<Node const> children() const { return m_children; }

	///
	/// \brief Transform for the node.
	///
	Transform transform{};
	///
	/// \brief For instanced rendering.
	///
	std::vector<Transform> instances{};
	///
	/// \brief Name of the node.
	///
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

	std::unordered_map<TypeId, std::unique_ptr<Base>, Hasher> m_attachments{};
	std::vector<Node> m_children{};
	Id<Node> m_id{};

	friend class Scene;
	friend class SceneRenderer;
};
// } // namespace refactor
} // namespace facade
