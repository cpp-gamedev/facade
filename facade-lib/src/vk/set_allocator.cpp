#include <facade/util/error.hpp>
#include <facade/vk/set_allocator.hpp>
#include <span>
#include <fmt/format.h>

namespace facade {
namespace {
vk::UniqueDescriptorSetLayout make_set_layout(vk::Device device, SetLayout const& layout) {
	auto const dslci = vk::DescriptorSetLayoutCreateInfo{{}, static_cast<std::uint32_t>(layout.bindings.span().size()), layout.bindings.span().data()};
	return device.createDescriptorSetLayoutUnique(dslci);
}

vk::UniqueDescriptorPool make_descriptor_pool(vk::Device device, SetLayout const& layout, std::uint32_t max_sets = 32) {
	auto pool_sizes = std::vector<vk::DescriptorPoolSize>{};
	for (auto const& binding : layout.bindings.span()) {
		auto& pool_size = pool_sizes.emplace_back();
		pool_size.type = binding.descriptorType;
		pool_size.descriptorCount = binding.descriptorCount;
	}
	if (pool_sizes.empty()) { return {}; }
	return device.createDescriptorPoolUnique({{}, max_sets, pool_sizes});
}
} // namespace

SetAllocator::SetAllocator(Gfx const& gfx, SetLayout const& layout, std::uint32_t number) : m_layout{layout}, m_gfx{gfx}, m_number{number} {
	m_set_layout = make_set_layout(m_gfx.device, m_layout);
	auto pool = make_descriptor_pool(m_gfx.device, m_layout);
	if (!pool) {
		m_empty = true;
		return;
	}
	m_pools.push_back(std::move(pool));
}

DescriptorSet& SetAllocator::acquire() {
	if (!m_gfx.device || m_empty) { throw Error{"Attempt to allocate descriptor set from empty allocator"}; }
	if (m_index >= m_sets.size()) {
		if (!try_allocate()) {
			m_pools.push_back(make_descriptor_pool(m_gfx.device, m_layout));
			if (!try_allocate()) { throw Error{"Failed to allocate descriptor set"}; }
		}
	}
	assert(m_index < m_sets.size());
	auto& ret = m_sets[m_index++];
	ret.clear();
	return ret;
}

bool SetAllocator::try_allocate() {
	static constexpr std::uint32_t count{8};
	vk::DescriptorSetLayout layouts[count]{};
	for (auto& layout : layouts) { layout = *m_set_layout; }
	auto& pool = m_pools.back();
	auto dsai = vk::DescriptorSetAllocateInfo{*pool, layouts};
	dsai.descriptorSetCount = count;
	vk::DescriptorSet sets[count]{};
	if (m_gfx.device.allocateDescriptorSets(&dsai, sets) != vk::Result::eSuccess) { return false; }
	m_sets.reserve(m_sets.size() + std::size(sets));
	for (auto set : sets) { m_sets.push_back({m_gfx, m_layout, set, m_number}); }
	return true;
}

SetAllocator::Pool::Pool(Gfx const& gfx, std::span<SetLayout const> layouts) : m_mutex{std::make_unique<std::mutex>()} {
	auto number = std::uint32_t{};
	for (auto const& layout : layouts) { m_sets.push_back({gfx, layout, number++}); }
}

auto SetAllocator::Pool::descriptor_set_layouts() const -> FlexArray<vk::DescriptorSetLayout, max_sets_v> {
	auto ret = FlexArray<vk::DescriptorSetLayout, max_sets_v>{};
	auto lock = std::scoped_lock{*m_mutex};
	for (auto const& set : m_sets) { ret.insert(set.descriptor_set_layout()); }
	return ret;
}

DescriptorSet& SetAllocator::Pool::next_set(std::uint32_t number) {
	if (!m_mutex) { throw Error{"Attempt to use empty set pool"}; }
	if (number >= m_sets.size()) { throw Error{fmt::format("OOB set number: [{}]", number)}; }
	auto lock = std::scoped_lock{*m_mutex};
	return m_sets[number].acquire();
}

void SetAllocator::Pool::release_all() {
	for (auto& set : m_sets) { set.release_all(); }
}
} // namespace facade
