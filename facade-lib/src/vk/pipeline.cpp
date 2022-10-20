#include <facade/vk/drawer.hpp>
#include <facade/vk/pipeline.hpp>

namespace facade {
void Pipeline::bind(vk::CommandBuffer cb) {
	m_cb = cb;
	m_cb.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
}

DescriptorSet& Pipeline::next_set(std::uint32_t number) const { return m_pool->next_set(number); }

void Pipeline::bind(DescriptorSet const& set) const { m_cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_layout, set.number(), set.set(), {}); }

void Pipeline::draw(StaticMesh const& mesh, std::span<glm::mat4x4 const> instances) const { m_drawer->draw(m_cb, mesh, instances); }
} // namespace facade
