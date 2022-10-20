#pragma once
#include <facade/vk/descriptor_set.hpp>
#include <span>

namespace facade {
class SetAllocator {
  public:
	class Pool;

  private:
	SetAllocator(Gfx const& gfx, SetLayout const& layout, std::uint32_t number);

	DescriptorSet& acquire();
	void release_all() { m_index = 0; }

	vk::DescriptorSetLayout const& descriptor_set_layout() const { return *m_set_layout; }

	bool try_allocate();

	SetLayout m_layout{};
	Gfx m_gfx{};
	vk::UniqueDescriptorSetLayout m_set_layout{};
	std::vector<vk::UniqueDescriptorPool> m_pools{};
	std::vector<DescriptorSet> m_sets{};
	std::size_t m_index{};
	std::uint32_t m_number{};
	bool m_empty{};
};

class SetAllocator::Pool {
  public:
	Pool(Gfx const& gfx, std::span<SetLayout const> layouts);

	FlexArray<vk::DescriptorSetLayout, max_sets_v> descriptor_set_layouts() const;
	DescriptorSet& next_set(std::uint32_t number);

	void release_all();

  private:
	std::vector<SetAllocator> m_sets{};
	std::unique_ptr<std::mutex> m_mutex{};
};
} // namespace facade
