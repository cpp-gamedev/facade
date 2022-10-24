#pragma once
#include <facade/util/ring_buffer.hpp>
#include <facade/vk/drawer.hpp>
#include <facade/vk/pipeline.hpp>
#include <facade/vk/shader.hpp>
#include <vulkan/vulkan_hash.hpp>
#include <mutex>
#include <optional>
#include <span>
#include <unordered_map>

namespace facade {
struct DrawInstance;
class StaticMesh;

class Pipes {
  public:
	using State = Pipeline::State;

	Pipes(Gfx const& gfx, vk::SampleCountFlagBits samples);

	[[nodiscard]] Pipeline get(vk::RenderPass rp, State const& state, Shader shader);

	void rotate();

  private:
	struct Key {
		State state{};
		std::size_t shader_hash{};
		bool operator==(Key const&) const = default;
	};
	struct Hasher {
		std::size_t operator()(Key const& key) const;
	};
	struct Map {
		std::unordered_map<vk::RenderPass, vk::UniquePipeline> pipelines{};
		std::vector<SetLayout> set_layouts{};
		RingBuffer<std::optional<SetAllocator::Pool>> set_pools{};
		vk::UniquePipelineLayout pipeline_layout{};
		bool populated{};
	};
	using Lock = std::scoped_lock<std::mutex>;

	void populate(Lock const&, Map& out, Shader shader) const;
	vk::UniquePipeline make_pipeline(State const& state, SpirV::View vert, SpirV::View frag, vk::PipelineLayout layout, vk::RenderPass rp) const;
	vk::UniquePipelineLayout make_pipeline_layout(std::span<vk::DescriptorSetLayout const> set_layouts) const;

	std::unordered_map<Key, Map, Hasher> m_map{};
	RingBuffer<Drawer> m_drawers{};
	Gfx m_gfx{};
	std::mutex m_mutex{};
	vk::SampleCountFlagBits m_samples{};
	bool m_sample_shading{};
};
} // namespace facade
