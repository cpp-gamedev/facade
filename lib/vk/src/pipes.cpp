#include <facade/util/error.hpp>
#include <facade/vk/geometry.hpp>
#include <facade/vk/pipes.hpp>
#include <facade/vk/static_mesh.hpp>
#include <spirv_glsl.hpp>
#include <map>

namespace facade {
namespace {
[[maybe_unused]] constexpr bool has_all_sets(std::span<SetLayout const> layouts) {
	std::uint32_t set{};
	for (auto const& layout : layouts) {
		if (layout.set != set) { return false; }
		++set;
	}
	return true;
}

std::vector<SetLayout> make_set_layouts(SpirV::View vert, SpirV::View frag) {
	std::map<std::uint32_t, std::map<std::uint32_t, vk::DescriptorSetLayoutBinding>> set_layout_bindings{};
	auto populate = [&set_layout_bindings](SpirV::View code, vk::ShaderStageFlagBits stage) {
		auto compiler = spirv_cross::CompilerGLSL{code.code.data(), code.code.size()};
		auto resources = compiler.get_shader_resources();
		auto set_resources = [&compiler, &set_layout_bindings, stage](std::span<spirv_cross::Resource const> resources, vk::DescriptorType const type) {
			for (auto const& resource : resources) {
				auto const set_number = compiler.get_decoration(resource.id, spv::Decoration::DecorationDescriptorSet);
				auto& layout = set_layout_bindings[set_number];
				auto const binding_number = compiler.get_decoration(resource.id, spv::Decoration::DecorationBinding);
				auto& dslb = layout[binding_number];
				dslb.binding = binding_number;
				dslb.descriptorType = type;
				dslb.stageFlags |= stage;
				auto const& type = compiler.get_type(resource.type_id);
				if (type.array.size() == 0) {
					dslb.descriptorCount = std::max(dslb.descriptorCount, 1U);
				} else {
					dslb.descriptorCount = type.array[0];
				}
			}
		};
		set_resources(resources.uniform_buffers, vk::DescriptorType::eUniformBuffer);
		set_resources(resources.storage_buffers, vk::DescriptorType::eStorageBuffer);
		set_resources(resources.sampled_images, vk::DescriptorType::eCombinedImageSampler);
	};
	populate(vert, vk::ShaderStageFlagBits::eVertex);
	populate(frag, vk::ShaderStageFlagBits::eFragment);

	auto ret = std::vector<SetLayout>{};
	auto next_set = std::uint32_t{};
	for (auto& [set, layouts] : set_layout_bindings) {
		while (next_set < set) {
			ret.push_back({});
			++next_set;
		}
		next_set = set + 1;
		auto layout = SetLayout{.set = set};
		for (auto const& [binding, dslb] : layouts) { layout.bindings.insert(dslb); }
		ret.push_back(std::move(layout));
	}
	assert(std::is_sorted(ret.begin(), ret.end(), [](SetLayout const& a, SetLayout const& b) { return a.set < b.set; }));
	assert(has_all_sets(ret));
	return ret;
}

vk::UniqueShaderModule make_shader_module(vk::Device device, SpirV::View spir_v) {
	auto smci = vk::ShaderModuleCreateInfo{};
	smci.codeSize = static_cast<std::uint32_t>(spir_v.code.size_bytes());
	smci.pCode = spir_v.code.data();
	return device.createShaderModuleUnique(smci);
}

struct VertexLayout {
	FlexArray<vk::VertexInputAttributeDescription, max_sets_v> attributes{};
	FlexArray<vk::VertexInputBindingDescription, max_sets_v> bindings{};
};

constexpr VertexLayout vertex_layout() {
	auto ret = VertexLayout{};
	ret.bindings.insert(vk::VertexInputBindingDescription{0, sizeof(Vertex)});
	ret.attributes.insert(vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)});
	ret.attributes.insert(vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, rgb)});
	ret.attributes.insert(vk::VertexInputAttributeDescription{2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)});
	ret.attributes.insert(vk::VertexInputAttributeDescription{3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)});
	ret.bindings.insert(vk::VertexInputBindingDescription{1, sizeof(glm::mat4x4), vk::VertexInputRate::eInstance});
	ret.attributes.insert(vk::VertexInputAttributeDescription{4, 1, vk::Format::eR32G32B32A32Sfloat, 0 * sizeof(glm::vec4)});
	ret.attributes.insert(vk::VertexInputAttributeDescription{5, 1, vk::Format::eR32G32B32A32Sfloat, 1 * sizeof(glm::vec4)});
	ret.attributes.insert(vk::VertexInputAttributeDescription{6, 1, vk::Format::eR32G32B32A32Sfloat, 2 * sizeof(glm::vec4)});
	ret.attributes.insert(vk::VertexInputAttributeDescription{7, 1, vk::Format::eR32G32B32A32Sfloat, 3 * sizeof(glm::vec4)});
	return ret;
}

struct PipeInfo {
	Pipes::State state{};
	vk::ShaderModule vert{};
	vk::ShaderModule frag{};
	vk::PipelineLayout layout{};
	vk::RenderPass render_pass{};

	vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
	bool sample_shading{true};
};

vk::UniquePipeline create_pipeline(vk::Device dv, PipeInfo const& info) {
	auto gpci = vk::GraphicsPipelineCreateInfo{};
	gpci.renderPass = info.render_pass;
	gpci.layout = info.layout;

	static constexpr auto vl = vertex_layout();
	auto const vertex_bindings = vl.bindings.span();
	auto const vertex_attributes = vl.attributes.span();
	auto pvisci = vk::PipelineVertexInputStateCreateInfo{};
	pvisci.pVertexBindingDescriptions = vertex_bindings.data();
	pvisci.vertexBindingDescriptionCount = static_cast<std::uint32_t>(vertex_bindings.size());
	pvisci.pVertexAttributeDescriptions = vertex_attributes.data();
	pvisci.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertex_attributes.size());
	gpci.pVertexInputState = &pvisci;

	auto pssciVert = vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, info.vert, "main");
	auto pssciFrag = vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, info.frag, "main");
	auto psscis = std::array<vk::PipelineShaderStageCreateInfo, 2>{{pssciVert, pssciFrag}};
	gpci.stageCount = static_cast<std::uint32_t>(psscis.size());
	gpci.pStages = psscis.data();

	auto piasci = vk::PipelineInputAssemblyStateCreateInfo({}, info.state.topology);
	gpci.pInputAssemblyState = &piasci;

	auto prsci = vk::PipelineRasterizationStateCreateInfo();
	prsci.polygonMode = info.state.mode;
	prsci.cullMode = vk::CullModeFlagBits::eNone;
	gpci.pRasterizationState = &prsci;

	auto pdssci = vk::PipelineDepthStencilStateCreateInfo{};
	pdssci.depthTestEnable = pdssci.depthWriteEnable = info.state.depth_test;
	pdssci.depthCompareOp = vk::CompareOp::eLess;
	gpci.pDepthStencilState = &pdssci;

	auto pcbas = vk::PipelineColorBlendAttachmentState{};
	using CCF = vk::ColorComponentFlagBits;
	pcbas.colorWriteMask = CCF::eR | CCF::eG | CCF::eB | CCF::eA;
	pcbas.blendEnable = true;
	pcbas.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	pcbas.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	pcbas.colorBlendOp = vk::BlendOp::eAdd;
	pcbas.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	pcbas.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	pcbas.alphaBlendOp = vk::BlendOp::eAdd;
	auto pcbsci = vk::PipelineColorBlendStateCreateInfo();
	pcbsci.attachmentCount = 1;
	pcbsci.pAttachments = &pcbas;
	gpci.pColorBlendState = &pcbsci;

	auto pdsci = vk::PipelineDynamicStateCreateInfo();
	vk::DynamicState const pdscis[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eLineWidth};
	pdsci = vk::PipelineDynamicStateCreateInfo({}, static_cast<std::uint32_t>(std::size(pdscis)), pdscis);
	gpci.pDynamicState = &pdsci;

	auto pvsci = vk::PipelineViewportStateCreateInfo({}, 1, {}, 1);
	gpci.pViewportState = &pvsci;

	auto pmssci = vk::PipelineMultisampleStateCreateInfo{};
	pmssci.rasterizationSamples = info.samples;
	pmssci.sampleShadingEnable = info.sample_shading;
	gpci.pMultisampleState = &pmssci;

	auto ret = vk::Pipeline{};
	if (dv.createGraphicsPipelines({}, 1U, &gpci, {}, &ret) != vk::Result::eSuccess) { throw Error{"Failed to create graphics pipeline"}; }

	return vk::UniquePipeline{ret, dv};
}
} // namespace

std::size_t Pipes::Hasher::operator()(Key const& key) const {
	auto ret = key.shader_hash;
	ret ^= (std::hash<vk::PolygonMode>{}(key.state.mode) << 16);
	ret ^= (std::hash<vk::PrimitiveTopology>{}(key.state.topology) << 24);
	ret ^= (std::hash<bool>{}(key.state.depth_test) << 32);
	return ret;
}

Pipes::Pipes(Gfx const& gfx, vk::SampleCountFlagBits samples) : m_gfx{gfx}, m_samples{samples} {
	auto const features = gfx.gpu.getFeatures();
	m_sample_shading = features.sampleRateShading;
	for (auto& drawer : m_drawers.t) { drawer = Drawer{gfx}; }
}

Pipeline Pipes::get(vk::RenderPass rp, State const& state, Shader shader) {
	auto const key = Key{.state = state, .shader_hash = std::hash<std::string_view>{}(shader.id)};
	auto lock = Lock{m_mutex};
	auto& map = m_map[key];
	populate(lock, map, shader);
	auto& ret = map.pipelines[rp];
	if (!ret) { ret = make_pipeline(state, shader.vert, shader.frag, *map.pipeline_layout, rp); }
	return {*ret, *map.pipeline_layout, &*map.set_pools.get(), &m_gfx.shared->device_limits, &m_drawers.get()};
}

void Pipes::rotate() {
	auto lock = std::scoped_lock{m_mutex};
	for (auto& [_, map] : m_map) {
		map.set_pools.get()->release_all();
		map.set_pools.rotate();
	}
	m_drawers.get().m_index = 0;
	m_drawers.rotate();
}

void Pipes::populate(Lock const&, Map& out, Shader shader) const {
	if (out.populated) { return; }
	out.set_layouts = make_set_layouts(shader.vert, shader.frag);
	for (auto& pool : out.set_pools.t) { pool.emplace(m_gfx, out.set_layouts); }
	out.pipeline_layout = make_pipeline_layout(out.set_pools.get()->descriptor_set_layouts().span());
	out.populated = true;
}

vk::UniquePipelineLayout Pipes::make_pipeline_layout(std::span<vk::DescriptorSetLayout const> set_layouts) const {
	auto plci = vk::PipelineLayoutCreateInfo{};
	plci.setLayoutCount = static_cast<std::uint32_t>(set_layouts.size());
	plci.pSetLayouts = set_layouts.data();
	return m_gfx.device.createPipelineLayoutUnique(plci);
}

vk::UniquePipeline Pipes::make_pipeline(State const& state, SpirV::View vert, SpirV::View frag, vk::PipelineLayout layout, vk::RenderPass rp) const {
	auto v = make_shader_module(m_gfx.device, vert);
	auto f = make_shader_module(m_gfx.device, frag);
	auto const info = PipeInfo{
		.state = state,
		.vert = *v,
		.frag = *f,
		.layout = layout,
		.render_pass = rp,
		.samples = m_samples,
		.sample_shading = m_sample_shading,
	};
	return create_pipeline(m_gfx.device, info);
}
} // namespace facade
