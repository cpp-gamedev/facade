#pragma once
#include <facade/util/fixed_string.hpp>
#include <facade/util/flex_array.hpp>
#include <facade/util/ptr.hpp>
#include <vulkan/vulkan.hpp>
#include <cassert>
#include <unordered_map>

namespace facade {
struct VertexLayout {
	class Db;
	using Id = FixedString<64>;

	static constexpr std::size_t max_v{16};

	Id id{};
	FlexArray<vk::VertexInputAttributeDescription, max_v> attributes{};
	FlexArray<vk::VertexInputBindingDescription, max_v> bindings{};
};

class VertexLayout::Db {
  public:
	VertexLayout const& add(VertexLayout const& layout) {
		assert(!layout.id.empty() && !layout.bindings.empty() && !layout.attributes.empty());
		auto [it, _] = m_map.insert_or_assign(layout.id, layout);
		return it->second;
	}

	Ptr<VertexLayout const> find(Id const& id) const {
		if (auto it = m_map.find(id); it != m_map.end()) { return &it->second; }
		return {};
	}

  private:
	std::unordered_map<Id, VertexLayout, std::hash<std::string_view>> m_map{};
};
} // namespace facade
