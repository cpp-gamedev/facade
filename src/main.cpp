
#include <facade/defines.hpp>

#include <facade/util/data_provider.hpp>
#include <facade/util/error.hpp>
#include <facade/util/fixed_string.hpp>
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

#include <iostream>

using namespace facade;

namespace facade::editor {
class Log : public Pinned, public logger::Accessor {
  public:
	struct Size {
		std::size_t max{};
		std::size_t extra{};
	};

	void render();

	bool show{};

  private:
	void operator()(std::span<logger::Entry const> entries) final;

	std::vector<logger::Entry const*> m_list{};
	EnumArray<logger::Level, bool> m_level_filter{true, true, true, debug_v};
	int m_display_count{50};
};

void Log::render() {
	ImGui::SetNextWindowSize({500.0f, 200.0f}, ImGuiCond_Once);
	if (auto window = editor::Window{"Log", &show}) {
		ImGui::Text("%s", FixedString{"Count: {}", m_display_count}.c_str());
		ImGui::SameLine();
		float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		ImGui::PushButtonRepeat(true);
		if (ImGui::ArrowButton("##left", ImGuiDir_Left)) { m_display_count = std::clamp(m_display_count - 10, 0, 1000); }
		ImGui::SameLine(0.0f, spacing);
		if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { m_display_count = std::clamp(m_display_count + 10, 0, 1000); }
		ImGui::PopButtonRepeat();

		static constexpr std::string_view levels_v[] = {"Error", "Warn", "Info", "Debug"};
		static constexpr auto max_log_level_v = debug_v ? logger::Level::eDebug : logger::Level::eInfo;
		for (logger::Level l = logger::Level::eError; l <= max_log_level_v; l = static_cast<logger::Level>(static_cast<int>(l) + 1)) {
			ImGui::SameLine();
			ImGui::Checkbox(levels_v[static_cast<std::size_t>(l)].data(), &m_level_filter[l]);
		}

		logger::access_buffer(*this);
		auto child = editor::Window{window, "scroll", {}, {}, ImGuiWindowFlags_HorizontalScrollbar};
		static constexpr auto im_colour = [](logger::Level const l) {
			switch (l) {
			case logger::Level::eError: return ImVec4{1.0f, 0.0f, 0.0f, 1.0f};
			case logger::Level::eWarn: return ImVec4{1.0f, 1.0f, 0.0f, 1.0f};
			default:
			case logger::Level::eInfo: return ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
			case logger::Level::eDebug: return ImVec4{0.5f, 0.5f, 0.5f, 1.0f};
			}
		};
		if (auto style = editor::StyleVar{ImGuiStyleVar_ItemSpacing, glm::vec2{}}) {
			for (auto const* entry : m_list) ImGui::TextColored(im_colour(entry->level), "%s", entry->message.c_str());
		}
	}
}

void Log::operator()(std::span<logger::Entry const> entries) {
	m_list.clear();
	for (auto const& entry : entries) {
		if (m_list.size() >= static_cast<std::size_t>(m_display_count)) { break; }
		if (!m_level_filter[entry.level]) { continue; }
		m_list.push_back(&entry);
	}
}
} // namespace facade::editor

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

struct MainMenu {
	struct {
		bool inspector{};
		bool stats{};
	} windows{};

	editor::Log log{};

	void inspector(Scene& scene) {
		ImGui::SetNextWindowSize({250.0f, 100.0f}, ImGuiCond_Once);
		if (auto window = editor::Window{"Node", &windows.inspector}) {
			static auto s_input = int{};
			ImGui::InputInt("Id", &s_input);
			editor::SceneInspector{window, scene}.inspect(Id<Node>{static_cast<std::size_t>(s_input)});
		}
	}

	void stats(Engine const& engine, float const dt) {
		ImGui::SetNextWindowSize({200.0f, 150.0f}, ImGuiCond_Once);
		if (auto window = editor::Window{"Frame Stats", &windows.stats}) {
			auto const& stats = engine.renderer().frame_stats();
			ImGui::Text("%s", FixedString{"Counter: {}", stats.frame_counter}.c_str());
			ImGui::Text("%s", FixedString{"Triangles: {}", stats.triangles}.c_str());
			ImGui::Text("%s", FixedString{"Draw calls: {}", stats.draw_calls}.c_str());
			ImGui::Text("%s", FixedString{"FPS: {}", (stats.fps == 0 ? static_cast<std::uint32_t>(stats.frame_counter) : stats.fps)}.c_str());
			ImGui::Text("%s", FixedString{"Frame time: {:.2f}ms", dt * 1000.0f}.c_str());
		}
	}

	void display(Engine& engine, Scene& scene, float const dt) {
		if (auto main = editor::MainMenu{}) {
			if (auto file = editor::Menu{main, "File"}) {
				ImGui::Separator();
				if (ImGui::MenuItem("Exit")) { engine.request_stop(); }
			}
			if (auto window = editor::Menu{main, "Window"}) {
				if (ImGui::MenuItem("Inspect")) { windows.inspector = true; }
				if (ImGui::MenuItem("Stats")) { windows.stats = true; }
				if (ImGui::MenuItem("Log")) { log.show = true; }
				ImGui::Separator();
				if (ImGui::MenuItem("Close All")) { windows = {}; }
			}
		}

		if (windows.inspector) { inspector(scene); }
		if (windows.stats) { stats(engine, dt); }
		if (log.show) { log.render(); }
	}
};

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

	auto main_menu = MainMenu{};

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

		main_menu.display(engine, scene, dt);
		// TEMP CODE

		engine.render(scene);
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
