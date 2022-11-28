#include <facade/vk/pipeline.hpp>

namespace facade {
void Pipeline::bind(vk::CommandBuffer cb) {
	m_cb = cb;
	m_cb.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
	set_line_width(1.0f);
}

DescriptorSet& Pipeline::next_set(std::uint32_t number) const { return m_pool->next_set(number); }

void Pipeline::set_line_width(float width) const { m_cb.setLineWidth(std::clamp(width, m_limits->lineWidthRange[0], m_limits->lineWidthRange[1])); }

void Pipeline::bind(DescriptorSet const& set) const { m_cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_layout, set.number(), set.set(), {}); }
} // namespace facade
