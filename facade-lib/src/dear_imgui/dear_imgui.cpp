#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <facade/dear_imgui/dear_imgui.hpp>
#include <facade/vk/cmd.hpp>

namespace facade {
namespace {
vk::UniqueDescriptorPool make_pool(vk::Device const device) {
	vk::DescriptorPoolSize pool_sizes[] = {
		{vk::DescriptorType::eSampledImage, 1000},		   {vk::DescriptorType::eCombinedImageSampler, 1000}, {vk::DescriptorType::eSampledImage, 1000},
		{vk::DescriptorType::eStorageImage, 1000},		   {vk::DescriptorType::eUniformTexelBuffer, 1000},	  {vk::DescriptorType::eStorageTexelBuffer, 1000},
		{vk::DescriptorType::eUniformBuffer, 1000},		   {vk::DescriptorType::eStorageBuffer, 1000},		  {vk::DescriptorType::eUniformBufferDynamic, 1000},
		{vk::DescriptorType::eStorageBufferDynamic, 1000}, {vk::DescriptorType::eInputAttachment, 1000},
	};
	auto pool_info = vk::DescriptorPoolCreateInfo{};
	pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	pool_info.maxSets = 1000 * std::size(pool_sizes);
	pool_info.poolSizeCount = static_cast<std::uint32_t>(std::size(pool_sizes));
	pool_info.pPoolSizes = pool_sizes;
	return device.createDescriptorPoolUnique(pool_info);
}
} // namespace

DearImgui::DearImgui(Info const& info) {
	m_pool = make_pool(info.gfx.device);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

	ImGui::StyleColorsDark();

	auto loader = vk::DynamicLoader{};
	auto get_fn = [&loader](char const* name) { return loader.getProcAddress<PFN_vkVoidFunction>(name); };
	auto lambda = +[](char const* name, void* ud) {
		auto const* gf = reinterpret_cast<decltype(get_fn)*>(ud);
		return (*gf)(name);
	};
	ImGui_ImplVulkan_LoadFunctions(lambda, &get_fn);
	ImGui_ImplGlfw_InitForVulkan(info.window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = info.gfx.instance;
	init_info.PhysicalDevice = info.gfx.gpu;
	init_info.Device = info.gfx.device;
	init_info.QueueFamily = info.gfx.queue_family;
	init_info.Queue = info.gfx.queue;
	init_info.DescriptorPool = *m_pool;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.MSAASamples = static_cast<VkSampleCountFlagBits>(info.samples);

	ImGui_ImplVulkan_Init(&init_info, info.render_pass);

	{
		auto cmd = Cmd{info.gfx};
		ImGui_ImplVulkan_CreateFontsTexture(cmd.cb);
	}
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

DearImgui::~DearImgui() {
	if (!m_pool) { return; }
	m_pool.getOwner().waitIdle();
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void DearImgui::new_frame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void DearImgui::render(vk::CommandBuffer const cb) { ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb); }
} // namespace facade
