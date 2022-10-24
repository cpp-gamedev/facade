
#include <facade/defines.hpp>

#include <facade/util/data_provider.hpp>
#include <facade/util/error.hpp>
#include <facade/util/fixed_string.hpp>
#include <facade/util/logger.hpp>
#include <facade/vk/geometry.hpp>

#include <djson/json.hpp>

#include <facade/render/renderer.hpp>
#include <facade/scene/fly_cam.hpp>

#include <facade/facade.hpp>

#include <facade/editor/inspector.hpp>
#include <facade/editor/log.hpp>
#include <facade/editor/scene_tree.hpp>

#include <bin/shaders.hpp>

#include <iostream>

#include <imgui.h>

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

struct MainMenu {
	struct {
		bool stats{};
		bool tree{};
	} windows{};

	editor::Log log{};
	struct {
		FixedString<128> name{};
		Id<Node> id{};
	} inspecting{};

	editor::Inspectee inspectee{};

	static constexpr std::string_view vsync_status(vk::PresentModeKHR const mode) {
		switch (mode) {
		case vk::PresentModeKHR::eFifo: return "On";
		case vk::PresentModeKHR::eFifoRelaxed: return "Adaptive";
		case vk::PresentModeKHR::eImmediate: return "Off";
		case vk::PresentModeKHR::eMailbox: return "Double-buffered";
		default: return "Unsupported";
		}
	}

	void change_vsync(Engine const& engine) const {
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

	void inspector(Scene& scene) {
		bool show = true;
		ImGui::SetNextWindowSize({400.0f, 400.0f}, ImGuiCond_Once);
		if (auto window = editor::Window{inspectee.name.c_str(), &show}) { editor::SceneInspector{window, scene}.inspect(inspectee.id); }
		if (!show) { inspectee = {}; }
	}

	void stats(Engine const& engine, float const dt) {
		ImGui::SetNextWindowSize({200.0f, 200.0f}, ImGuiCond_Once);
		if (auto window = editor::Window{"Frame Stats", &windows.stats}) {
			auto const& stats = engine.renderer().frame_stats();
			ImGui::Text("%s", FixedString{"Counter: {}", stats.frame_counter}.c_str());
			ImGui::Text("%s", FixedString{"Triangles: {}", stats.triangles}.c_str());
			ImGui::Text("%s", FixedString{"Draw calls: {}", stats.draw_calls}.c_str());
			ImGui::Text("%s", FixedString{"FPS: {}", (stats.fps == 0 ? static_cast<std::uint32_t>(stats.frame_counter) : stats.fps)}.c_str());
			ImGui::Text("%s", FixedString{"Frame time: {:.2f}ms", dt * 1000.0f}.c_str());
			if (ImGui::SmallButton("Vsync")) { change_vsync(engine); }
			ImGui::SameLine();
			ImGui::Text("%s", vsync_status(stats.mode).data());
		}
	}

	void tree(Scene& scene) {
		ImGui::SetNextWindowSize({250.0f, 350.0f}, ImGuiCond_Once);
		if (auto window = editor::Window{"Scene", &windows.tree}) { editor::SceneTree{scene}.render(window, inspectee); }
	}

	static FixedString<128> node_name(Node const& node) {
		auto ret = FixedString<128>{node.name};
		if (ret.empty()) { ret = "(Unnamed)"; }
		ret += FixedString{" ({})", node.id()};
		return ret;
	}

	void mark_inspect(Node const& node) {
		inspecting.id = node.id();
		inspecting.name = node_name(node);
		inspecting.name += FixedString{"###Node"};
	}

	void uninspect() { inspecting = {.name = "[Node]###Node"}; }

	void walk(Node& node) {
		auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (node.id() == inspecting.id) { flags |= ImGuiTreeNodeFlags_Selected; }
		if (node.children().empty()) { flags |= ImGuiTreeNodeFlags_Leaf; }
		auto tn = editor::TreeNode{node_name(node).c_str(), flags};
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			mark_inspect(node);
		} else if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			uninspect();
		}
		if (tn) {
			for (auto& child : node.children()) { walk(child); }
		}
	}

	void display(Engine& engine, Scene& scene, float const dt) {
		if (auto main = editor::MainMenu{}) {
			if (auto file = editor::Menu{main, "File"}) {
				ImGui::Separator();
				if (ImGui::MenuItem("Exit")) { engine.request_stop(); }
			}
			if (auto window = editor::Menu{main, "Window"}) {
				if (ImGui::MenuItem("Tree")) { windows.tree = true; }
				if (ImGui::MenuItem("Stats")) { windows.stats = true; }
				if (ImGui::MenuItem("Log")) { log.show = true; }
				ImGui::Separator();
				if (ImGui::MenuItem("Close All")) { windows = {}; }
			}
		}

		if (windows.tree) { tree(scene); }
		if (inspectee) { inspector(scene); }
		if (windows.stats) { stats(engine, dt); }
		if (log.show) { log.render(); }
	}
};

void run() {
	auto context = std::optional<Context>{};

	struct DummyDataProvider : DataProvider {
		ByteBuffer load(std::string_view) const override { return {}; }
	};

	auto material_id = Id<Material>{};
	auto node_id = Id<Node>{};
	auto post_scene_load = [&] {
		context->scene.camera().transform.set_position({0.0f, 0.0f, 5.0f});

		auto material = std::make_unique<LitMaterial>();
		material->albedo = {1.0f, 0.0f, 0.0f};
		material_id = context->scene.add(std::move(material));
		auto static_mesh_id = context->scene.add(StaticMesh{context->engine.gfx(), make_cubed_sphere(1.0f, 32)});
		auto mesh_id = context->scene.add(Mesh{.primitives = {Mesh::Primitive{static_mesh_id, material_id}}});

		auto node = Node{};
		node.attach(mesh_id);
		node.instances.emplace_back().set_position({1.0f, -5.0f, -20.0f});
		node.instances.emplace_back().set_position({-1.0f, 1.0f, 0.0f});
		node_id = context->scene.add(std::move(node), 0);
		context->show(true);
	};

	auto init = [&] {
		context.emplace();
		auto lit = shaders::lit();
		lit.id = "default";
		context->add_shader(lit);
		context->add_shader(shaders::unlit());

		context->scene.dir_lights.push_back(DirLight{.direction = glm::normalize(glm::vec3{-1.0f, -1.0f, -1.0f}), .diffuse = glm::vec3{5.0f}});
		context->scene.load_gltf(dj::Json::parse(test_json_v), DummyDataProvider{});
		post_scene_load();
	};

	init();

	float const drot_z[] = {100.0f, -150.0f};

	auto main_menu = MainMenu{};

	while (context->running()) {
		auto const dt = context->next_frame();
		auto const& state = context->engine.window().state();
		auto const& input = state.input;
		bool const mouse_look = input.mouse.held(GLFW_MOUSE_BUTTON_RIGHT);

		if (input.keyboard.pressed(GLFW_KEY_ESCAPE)) { context->request_stop(); }
		glfwSetInputMode(context->engine.window(), GLFW_CURSOR, mouse_look ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

		if (!state.file_drops.empty()) {
			auto const& file = state.file_drops.front();
			if (!load_gltf(context->scene, file)) {
				logger::warn("Failed to load GLTF: [{}]", file);
			} else {
				post_scene_load();
			}
		}

		auto& camera = context->scene.camera();
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
			context.reset();
			init();
			continue;
		}

		// TEMP CODE
		auto* node = context->scene.find_node(node_id);
		node->instances[0].rotate(glm::radians(drot_z[0]) * dt, {0.0f, 1.0f, 0.0f});
		node->instances[1].rotate(glm::radians(drot_z[1]) * dt, {1.0f, 0.0f, 0.0f});

		ImGui::ShowDemoWindow();

		main_menu.display(context->engine, context->scene, dt);
		// TEMP CODE
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
