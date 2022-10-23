
#include <facade/defines.hpp>

#include <facade/util/data_provider.hpp>
#include <facade/util/error.hpp>
#include <facade/util/geometry.hpp>
#include <facade/util/logger.hpp>

#include <facade/dear_imgui/dear_imgui.hpp>

#include <djson/json.hpp>

#include <facade/engine/engine.hpp>
#include <facade/engine/renderer.hpp>

#include <facade/scene/fly_cam.hpp>
#include <facade/scene/scene.hpp>

#include <facade/editor/inspector.hpp>

#include <bin/shaders.hpp>

using namespace facade;

namespace {
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
	auto engine = Engine{};

	auto lit = shaders::lit();
	lit.id = "default";
	engine.add_shader(lit);
	engine.add_shader(shaders::unlit());

	struct DummyDataProvider : DataProvider {
		ByteBuffer load(std::string_view) const override { return {}; }
	};

	auto scene = Scene{engine.gfx()};
	scene.dir_lights.push_back(DirLight{.direction = glm::normalize(glm::vec3{-1.0f, -1.0f, -1.0f}), .diffuse = glm::vec3{5.0f}});

	auto material_id = Id<Material>{};
	auto node_id = Id<Node>{};
	auto post_scene_load = [&] {
		scene.camera().transform.set_position({0.0f, 0.0f, 5.0f});

		auto material = std::make_unique<LitMaterial>();
		material->albedo = {1.0f, 0.0f, 0.0f};
		material_id = scene.add(std::move(material));
		auto static_mesh_id = scene.add(StaticMesh{engine.gfx(), make_cubed_sphere(1.0f, 32)});
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

	engine.show(true);
	while (engine.running()) {
		auto const dt = engine.next_frame();
		auto const& state = engine.window().state();
		auto const& input = state.input;
		bool const mouse_look = input.mouse.held(GLFW_MOUSE_BUTTON_RIGHT);

		if (input.keyboard.pressed(GLFW_KEY_ESCAPE)) { engine.request_stop(); }
		glfwSetInputMode(engine.window(), GLFW_CURSOR, mouse_look ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

		if (!state.file_drops.empty()) {
			auto const& file = state.file_drops.front();
			if (!load_gltf(scene, file)) {
				logger::warn("Failed to load GLTF: [{}]", file);
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
			if (input.keyboard.held(GLFW_KEY_E)) { fly_cam.move_up(dt); }
			if (input.keyboard.held(GLFW_KEY_Q)) { fly_cam.move_up(-dt); }
			if (mouse_look) { fly_cam.rotate({input.mouse.delta_pos().x, -input.mouse.delta_pos().y}); }
		}

		auto* node = scene.find_node(node_id);
		node->instances[0].rotate(glm::radians(drot_z[0]) * dt, {0.0f, 1.0f, 0.0f});
		node->instances[1].rotate(glm::radians(drot_z[1]) * dt, {1.0f, 0.0f, 0.0f});

		// TEMP CODE
		if (input.keyboard.pressed(GLFW_KEY_M)) {
			static constexpr vk::PresentModeKHR modes[] = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eFifoRelaxed, vk::PresentModeKHR::eMailbox,
														   vk::PresentModeKHR::eImmediate};
			static std::size_t index{0};
			auto const next_mode = [&] {
				while (true) {
					index = (index + 1) % std::size(modes);
					auto const ret = modes[index];
					if (!engine.renderer().is_supported(ret)) { continue; }
					return ret;
				}
				throw Error{"Invariant violated"};
			}();
			engine.renderer().request_mode(next_mode);
			logger::info("Requesting present mode: [{}]", present_mode_str(next_mode));
		}

		ImGui::ShowDemoWindow();

		ImGui::SetNextWindowSize({250.0f, 100.0f}, ImGuiCond_Once);
		if (auto window = editor::Window{"Node"}) {
			static auto s_input = int{};
			ImGui::InputInt("Id", &s_input);
			editor::SceneInspector{window, scene}.inspect(Id<Node>{static_cast<std::size_t>(s_input)});
		}

		ImGui::SetNextWindowSize({200.0f, 150.0f}, ImGuiCond_Once);
		if (auto window = editor::Window("Frame Stats")) {
			auto const& stats = engine.renderer().frame_stats();
			ImGui::Text("Counter: %lu", stats.frame_counter);
			ImGui::Text("Triangles: %lu", stats.triangles);
			ImGui::Text("Draw calls: %u", stats.draw_calls);
			ImGui::Text("FPS: %u", (stats.fps == 0 ? static_cast<std::uint32_t>(stats.frame_counter) : stats.fps));
			ImGui::Text("Frame time: %.2fms", dt * 1000.0f);
		}
		// TEMP CODE

		engine.render(scene);
	}
}
} // namespace

int main() {
	try {
		run();
	} catch (InitError const& e) {
		logger::error("Initialization failure: {}", e.what());
		return EXIT_FAILURE;
	} catch (Error const& e) {
		logger::error("Runtime error: {}", e.what());
		return EXIT_FAILURE;
	} catch (std::exception const& e) {
		logger::error("Fatal error: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		logger::error("Unknown error");
		return EXIT_FAILURE;
	}
}
