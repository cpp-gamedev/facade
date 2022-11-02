#include <facade/defines.hpp>

#include <facade/util/data_provider.hpp>
#include <facade/util/error.hpp>
#include <facade/util/logger.hpp>
#include <facade/vk/geometry.hpp>

#include <facade/scene/fly_cam.hpp>

#include <facade/engine/engine.hpp>

#include <bin/shaders.hpp>
#include <main_menu/main_menu.hpp>

#include <djson/json.hpp>

#include <imgui.h>

#include <filesystem>
#include <iostream>

using namespace facade;

namespace {
namespace fs = std::filesystem;

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

void log_prologue() {
	auto const now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	char buf[32]{};
	std::strftime(buf, sizeof(buf), "%F %Z", std::localtime(&now));
	static constexpr auto v = version_v;
	logger::info("facade v{}.{}.{} | {} |", v.major, v.minor, v.patch, buf);
}

fs::path find_gltf(fs::path root) {
	if (root.extension() == ".gltf") { return root; }
	if (!fs::is_directory(root)) { return {}; }
	for (auto const& it : fs::directory_iterator{root}) {
		if (!it.is_regular_file()) { continue; }
		auto path = it.path();
		if (path.extension() == ".gltf") { return path; }
	}
	return {};
}

void run() {
	auto engine = std::optional<Engine>{};

	struct DummyDataProvider : DataProvider {
		ByteBuffer load(std::string_view) const override { return {}; }
	};

	auto material_id = Id<Material>{};
	auto node_id = Id<Node>{};
	auto post_scene_load = [&]() {
		auto& scene = engine->scene();
		scene.lights.dir_lights.insert(DirLight{.direction = glm::normalize(glm::vec3{-1.0f, -1.0f, -1.0f}), .rgb = {.intensity = 5.0f}});
		scene.camera().transform.set_position({0.0f, 0.0f, 5.0f});

		auto material = std::make_unique<LitMaterial>();
		material->albedo = {1.0f, 0.0f, 0.0f};
		material_id = scene.add(std::move(material));
		auto static_mesh_id = scene.add(make_cubed_sphere(1.0f, 32));
		// auto static_mesh_id = scene.add(make_manipulator(0.125f, 1.0f, 16));
		auto mesh_id = scene.add(Mesh{.primitives = {Mesh::Primitive{static_mesh_id, material_id}}});

		auto node = Node{};
		node.attach(mesh_id);
		node.instances.emplace_back().set_position({1.0f, -5.0f, -20.0f});
		node.instances.emplace_back().set_position({-1.0f, 1.0f, 0.0f});
		node_id = scene.add(std::move(node), 0);
	};

	auto init = [&] {
		engine.emplace();
		log_prologue();

		auto lit = shaders::lit();
		lit.id = "default";
		engine->add_shader(lit);
		engine->add_shader(shaders::unlit());

		auto& scene = engine->scene();
		scene.load_gltf(dj::Json::parse(test_json_v), DummyDataProvider{});
		post_scene_load();
		engine->show(true);
	};

	init();

	float const drot_z[] = {100.0f, -150.0f};

	auto file_menu = FileMenu{};
	auto window_menu = WindowMenu{};

	struct {
		LoadStatus status{};
		std::string title{};
	} loading{};

	while (engine->running()) {
		auto const& state = engine->poll();
		auto const& input = state.input;
		auto const dt = state.dt;
		bool const mouse_look = input.mouse.held(GLFW_MOUSE_BUTTON_RIGHT);

		if (input.keyboard.pressed(GLFW_KEY_ESCAPE)) { engine->request_stop(); }
		glfwSetInputMode(engine->window(), GLFW_CURSOR, mouse_look ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

		if (!state.file_drops.empty() && engine->load_status().total == 0) {
			auto path = find_gltf(state.file_drops.front());
			if (!fs::is_regular_file(path)) {
				logger::error("Failed to locate .gltf in path: [{}]", state.file_drops.front());
			} else {
				if (engine->load_async(path.generic_string(), post_scene_load)) {
					loading.title = fmt::format("Loading {}...", path.filename().generic_string());
					file_menu.add_recent(path.generic_string());
				}
			}
		}
		loading.status = engine->load_status();

		if (loading.status.stage > LoadStage::eNone) {
			auto const* main_viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos({0.0f, main_viewport->WorkPos.y + main_viewport->Size.y - 100.0f});
			ImGui::SetNextWindowSize({main_viewport->Size.x, 100.0f});
			auto window = editor::Window{loading.title.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize};
			ImGui::Text("%s", load_stage_str[loading.status.stage].data());
			auto const progress = FixedString{"{} / {}", loading.status.done, loading.status.total};
			ImGui::ProgressBar(loading.status.load_progress(), ImVec2{-1.0f, 0.0f}, progress.c_str());
		}

		auto& camera = engine->scene().camera();
		if (auto fly_cam = FlyCam{camera.transform}) {
			if (input.keyboard.held(GLFW_KEY_A) || input.keyboard.held(GLFW_KEY_LEFT)) { fly_cam.move_right(-dt); }
			if (input.keyboard.held(GLFW_KEY_D) || input.keyboard.held(GLFW_KEY_RIGHT)) { fly_cam.move_right(dt); }
			if (input.keyboard.held(GLFW_KEY_W) || input.keyboard.held(GLFW_KEY_UP)) { fly_cam.move_front(-dt); }
			if (input.keyboard.held(GLFW_KEY_S) || input.keyboard.held(GLFW_KEY_DOWN)) { fly_cam.move_front(dt); }
			if (input.keyboard.held(GLFW_KEY_E)) { fly_cam.move_up(dt); }
			if (input.keyboard.held(GLFW_KEY_Q)) { fly_cam.move_up(-dt); }
			if (mouse_look) { fly_cam.rotate({input.mouse.delta_pos().x, -input.mouse.delta_pos().y}); }
		}

		if (input.keyboard.pressed(GLFW_KEY_R)) {
			logger::info("Reloading...");
			engine.reset();
			init();
			continue;
		}

		// TEMP CODE
		if (auto* node = engine->scene().find(node_id)) {
			node->instances[0].rotate(glm::radians(drot_z[0]) * dt, {0.0f, 1.0f, 0.0f});
			node->instances[1].rotate(glm::radians(drot_z[1]) * dt, {1.0f, 0.0f, 0.0f});
		}

		if (auto menu = editor::MainMenu{}) {
			auto file_command = file_menu.display(menu);
			window_menu.display_menu(menu);
			window_menu.display_windows(*engine);
			auto const visitor = Visitor{
				[&engine](FileMenu::Shutdown) { engine->request_stop(); },
				[&engine, &post_scene_load, &file_menu](FileMenu::OpenRecent open_recent) {
					engine->load_async(open_recent.path, post_scene_load);
					file_menu.add_recent(std::move(open_recent.path));
				},
				[](std::monostate) {},
			};
			std::visit(visitor, file_command);
		}
		// TEMP CODE

		engine->render();
	}
}
} // namespace

int main() {
	try {
		auto logger_instance = logger::Instance{};
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
	} catch (std::exception const& e) { std::cerr << e.what() << '\n'; }
}
