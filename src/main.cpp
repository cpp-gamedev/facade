
#include <glm/gtx/quaternion.hpp>

#include <facade/defines.hpp>

#include <facade/util/data_provider.hpp>
#include <facade/util/error.hpp>
#include <facade/util/geometry.hpp>
#include <facade/util/image.hpp>
#include <facade/util/logger.hpp>
#include <facade/util/time.hpp>

#include <facade/glfw/glfw.hpp>

#include <facade/vk/buffer.hpp>
#include <facade/vk/static_mesh.hpp>
#include <facade/vk/texture.hpp>
#include <facade/vk/vk.hpp>

#include <facade/dear_imgui/dear_imgui.hpp>

#include <djson/json.hpp>

#include <facade/scene/camera.hpp>
#include <facade/scene/fly_cam.hpp>
#include <facade/scene/material.hpp>
#include <facade/scene/renderer.hpp>
#include <facade/scene/scene.hpp>
#include <facade/scene/transform.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <bin/shaders.hpp>
#include <fstream>

using namespace facade;

namespace {
[[maybe_unused]] ByteBuffer read_file(char const* path) {
	auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
	if (!file) { return {}; }
	auto const ssize = file.tellg();
	file.seekg({});
	auto ret = ByteBuffer::make(static_cast<std::size_t>(ssize));
	file.read(reinterpret_cast<char*>(ret.data()), ssize);
	return ret;
}

UniqueWin make_window() {
	auto ret = Glfw::Window::make();
	glfwSetWindowTitle(ret.get(), "facade");
	glfwSetWindowSize(ret.get(), 800, 800);
	return ret;
}

Vulkan make_vulkan(Glfw::Window const& window) {
	auto ret = Vulkan{};
	ret.instance = Vulkan::Instance::make(window.glfw->vk_extensions(), Vulkan::eValidation);
	auto surface = window.make_surface(*ret.instance.instance);
	ret.device = Vulkan::Device::make(ret.instance, *surface);
	ret.vma = Vulkan::make_vma(*ret.instance.instance, ret.device.gpu.device, *ret.device.device);
	ret.shared = std::make_unique<Gfx::Shared>();
	ret.shared->device_limits = ret.device.gpu.properties.limits;
	return ret;
}

bool load_gltf(Scene& out, std::string_view path) {
	auto const provider = FileDataProvider::mount_parent_dir(path);
	auto json = dj::Json::from_file(path.data());
	return out.load_gltf(json, provider);
}

static constexpr auto test_json_v = R"(
{
  "scene": 0,
  "scenes" : [
    {
      "nodes" : [ 0 ]
    }
  ],

  "nodes" : [
    {
      "mesh" : 0
    }
  ],

  "meshes" : [
    {
      "primitives" : [ {
        "attributes" : {
          "POSITION" : 1
        },
        "indices" : 0,
        "material" : 0
      } ]
    }
  ],

  "buffers" : [
    {
      "uri" : "data:application/octet-stream;base64,AAABAAIAAAAAAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAAAAAACAPwAAAAA=",
      "byteLength" : 44
    }
  ],
  "bufferViews" : [
    {
      "buffer" : 0,
      "byteOffset" : 0,
      "byteLength" : 6,
      "target" : 34963
    },
    {
      "buffer" : 0,
      "byteOffset" : 8,
      "byteLength" : 36,
      "target" : 34962
    }
  ],
  "accessors" : [
    {
      "bufferView" : 0,
      "byteOffset" : 0,
      "componentType" : 5123,
      "count" : 3,
      "type" : "SCALAR",
      "max" : [ 2 ],
      "min" : [ 0 ]
    },
    {
      "bufferView" : 1,
      "byteOffset" : 0,
      "componentType" : 5126,
      "count" : 3,
      "type" : "VEC3",
      "max" : [ 1.0, 1.0, 0.0 ],
      "min" : [ 0.0, 0.0, 0.0 ]
    }
  ],

  "materials" : [
    {
      "pbrMetallicRoughness": {
        "baseColorFactor": [ 1.000, 0.766, 0.336, 1.0 ],
        "metallicFactor": 0.5,
        "roughnessFactor": 0.1
      }
    }
  ],
  "asset" : {
    "version" : "2.0"
  }
}
)";

void run() {
	auto window = make_window();
	auto vulkan = make_vulkan(window);
	auto gfx = vulkan.gfx();

	auto const renderer_info = Renderer::Info{.command_buffers = 1, .samples = vk::SampleCountFlagBits::e1};
	auto renderer = Renderer{vulkan.gfx(), window, renderer_info};

	auto lit = shaders::lit();
	lit.id = "default";
	renderer.add_shader(lit);
	renderer.add_shader(shaders::unlit());

	struct DummyDataProvider : DataProvider {
		ByteBuffer load(std::string_view) const override { return {}; }
	};

	auto scene = Scene{gfx};
	scene.dir_lights.push_back(DirLight{.direction = glm::normalize(glm::vec3{-1.0f, -1.0f, -1.0f}), .diffuse = glm::vec3{5.0f}});

	auto material_id = Id<Material>{};
	auto node_id = Id<Node>{};
	auto post_scene_load = [&] {
		scene.camera().transform.set_position({0.0f, 0.0f, 5.0f});

		auto material = std::make_unique<TestMaterial>();
		material->albedo = {1.0f, 0.0f, 0.0f};
		material_id = scene.add(std::move(material));
		auto static_mesh_id = scene.add(StaticMesh{gfx, make_cubed_sphere(1.0f, 32)});
		auto mesh_id = scene.add(Mesh{.primitives = {Mesh::Primitive{static_mesh_id, material_id}}});

		auto node = Node{};
		node.attach(mesh_id);
		node.instances.emplace_back().set_position({1.0f, -5.0f, -20.0f});
		node.instances.emplace_back().set_position({-1.0f, 1.0f, 0.0f});
		node_id = scene.add(std::move(node), 0);
	};

	scene.load_gltf(dj::Json::parse(test_json_v), DummyDataProvider{});
	post_scene_load();

	float const drot_z[] = {100.0f, -150.0f};

	auto delta_time = DeltaTime{};
	glfwShowWindow(window.get());
	while (!glfwWindowShouldClose(window.get())) {
		auto const dt = delta_time();

		window.get().glfw->poll_events();

		auto const& input = window.get().state().input;
		bool const mouse_look = input.mouse.held(GLFW_MOUSE_BUTTON_1);

		if (input.keyboard.pressed(GLFW_KEY_ESCAPE)) { glfwSetWindowShouldClose(window.get(), GLFW_TRUE); }
		glfwSetInputMode(window.get(), GLFW_CURSOR, mouse_look ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

		auto cb = vk::CommandBuffer{};
		if (!renderer.next_frame({&cb, 1})) { continue; }

		if (auto const& files = window.get().state().file_drops; !files.empty()) {
			auto const& file = files.front();
			if (!load_gltf(scene, file)) {
				logger::warn("Failed to load GLTF: ", file);
			} else {
				post_scene_load();
			}
		}

		auto& camera = scene.camera();
		if (auto fly_cam = FlyCam{camera.transform}) {
			if (input.keyboard.held(GLFW_KEY_A) || input.keyboard.held(GLFW_KEY_LEFT)) { fly_cam.move_right(-dt); }
			if (input.keyboard.held(GLFW_KEY_D) || input.keyboard.held(GLFW_KEY_RIGHT)) { fly_cam.move_right(dt); }
			if (input.keyboard.held(GLFW_KEY_W) || input.keyboard.held(GLFW_KEY_UP)) { fly_cam.move_front(-dt); }
			if (input.keyboard.held(GLFW_KEY_S) || input.keyboard.held(GLFW_KEY_DOWN)) { fly_cam.move_front(dt); }
			if (mouse_look) { fly_cam.rotate({input.mouse.delta_pos().x, -input.mouse.delta_pos().y}); }
		}

		auto* node = scene.find_node(node_id);
		node->instances[0].rotate(glm::radians(drot_z[0]) * dt, {0.0f, 1.0f, 0.0f});
		node->instances[1].rotate(glm::radians(drot_z[1]) * dt, {1.0f, 0.0f, 0.0f});

		ImGui::ShowDemoWindow();

		ImGui::SetNextWindowSize({250.0f, 100.0f}, ImGuiCond_Once);
		if (ImGui::Begin("Material")) {
			auto* mat = static_cast<TestMaterial*>(scene.find_material(material_id));
			ImGui::SliderFloat("Metallic", &mat->metallic, 0.0f, 1.0f);
			ImGui::SliderFloat("Roughness", &mat->roughness, 0.0f, 1.0f);
		}
		ImGui::End();

		scene.render(renderer, cb);

		renderer.render();
	}

	gfx.device.waitIdle();
}
} // namespace

int main() {
	try {
		run();
	} catch (InitError const& e) {
		logger::error("Initialization failure: ", e.what());
		return EXIT_FAILURE;
	} catch (Error const& e) {
		logger::error("Runtime error: ", e.what());
		return EXIT_FAILURE;
	} catch (std::exception const& e) {
		logger::error("Fatal error: ", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		logger::error("Unknown error");
		return EXIT_FAILURE;
	}
}
