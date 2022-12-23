#pragma once
#include <facade/vk/pipeline.hpp>
#include <facade/vk/rotator.hpp>
#include <facade/vk/shader.hpp>
#include <facade/vk/vertex_layout.hpp>
#include <vulkan/vulkan_hash.hpp>
#include <mutex>
#include <optional>
#include <span>
#include <unordered_map>

namespace facade {
class Pipes {
  public:
	using State = Pipeline::State;

	Pipes(Gfx const& gfx, vk::SampleCountFlagBits samples);

	[[nodiscard]] Pipeline get(vk::RenderPass rp, State const& state, VertexInput const& vinput, Shader::Program const& shader);

	void rotate();

  private:
	struct Key {
		State state{};
		std::size_t shader_hash{};
		std::size_t vertex_input_hash{};
		bool operator==(Key const&) const = default;
	};
	struct Hasher {
		std::size_t operator()(Key const& key) const;
	};
	struct Map {
		std::unordered_map<vk::RenderPass, vk::UniquePipeline> pipelines{};
		std::vector<SetLayout> set_layouts{};
		Rotator<std::optional<SetAllocator::Pool>> set_pools{};
		vk::UniquePipelineLayout pipeline_layout{};
		bool populated{};
	};
	using Lock = std::scoped_lock<std::mutex>;

	void populate(Lock const&, Map& out, Shader::Program const& shader) const;
	vk::UniquePipeline make_pipeline(State const&, VertexInput const&, SpirV::View vert, SpirV::View frag, vk::PipelineLayout, vk::RenderPass) const;
	vk::UniquePipelineLayout make_pipeline_layout(std::span<vk::DescriptorSetLayout const> set_layouts) const;

	std::unordered_map<Key, Map, Hasher> m_map{};
	Gfx m_gfx{};
	std::mutex m_mutex{};
	vk::SampleCountFlagBits m_samples{};
	bool m_sample_shading{};
};
} // namespace facade
