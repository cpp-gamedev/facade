#include <facade/defines.hpp>

#include <facade/util/cli_opts.hpp>
#include <facade/util/data_provider.hpp>
#include <facade/util/error.hpp>
#include <facade/util/logger.hpp>

#include <facade/vk/geometry.hpp>

#include <facade/scene/fly_cam.hpp>

#include <facade/engine/editor/browse_file.hpp>
#include <facade/engine/engine.hpp>

#include <bin/shaders.hpp>
#include <config/config.hpp>
#include <events/events.hpp>
#include <gui/gltf_sources.hpp>
#include <gui/main_menu.hpp>

#include <imgui.h>

#include <filesystem>
#include <iostream>

using namespace facade;

namespace {
namespace fs = std::filesystem;

std::string_view version_string() {
	static auto const ret = fmt::format("v{}.{}.{}", version_v.major, version_v.minor, version_v.patch);
	return ret;
}

void log_prologue() {
	auto const now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	char buf[32]{};
	std::strftime(buf, sizeof(buf), "%F %Z", std::localtime(&now));
	logger::info("facade {} | {} |", version_string(), buf);
}

struct AppOpts {
	std::optional<std::uint32_t> force_threads{};
};

void run(AppOpts const& opts) {
	auto events = std::make_shared<Events>();
	auto engine = std::optional<Engine>{};
	auto engine_info = Engine::CreateInfo{};
	auto config = Config::Scoped{};
	engine_info.extent = config.config.window.extent;
	engine_info.desired_msaa = config.config.window.msaa;
	engine_info.force_thread_count = opts.force_threads;

	auto node_id = Id<Node>{};
	auto post_scene_load = [&engine, &node_id]() {
		auto& scene = engine->scene();
		scene.lights.dir_lights.insert(DirLight{.direction = glm::normalize(glm::vec3{-1.0f, -1.0f, -1.0f}), .rgb = {.intensity = 5.0f}});
		scene.camera().transform.set_position({0.0f, 0.0f, 5.0f});

		auto material = std::make_unique<LitMaterial>();
		material->albedo = {1.0f, 0.0f, 0.0f};
		auto material_id = scene.add(Material{std::move(material)});
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
		engine.emplace(engine_info);
		if (config.config.window.position) { glfwSetWindowPos(engine->window(), config.config.window.position->x, config.config.window.position->y); }
		log_prologue();

		auto lit = shaders::lit();
		lit.id = "default";
		engine->add_shader(lit);
		engine->add_shader(shaders::unlit());

		post_scene_load();
		engine->show(true);
	};

	init();

	float const drot_z[] = {100.0f, -150.0f};

	auto file_menu = FileMenu{};
	auto window_menu = WindowMenu{};

	for (auto const& recent : config.config.file_menu.recents) { file_menu.add_recent(recent); }

	struct {
		LoadStatus status{};
		std::string title{};
	} loading{};

	auto load_async = [&engine, &file_menu, &loading, &post_scene_load, &config](fs::path const& path) {
		if (engine->load_async(path.generic_string(), post_scene_load)) {
			loading.title = fmt::format("Loading {}...", path.filename().generic_string());
			file_menu.add_recent(path.generic_string());
			config.config.file_menu.recents = {file_menu.recents().begin(), file_menu.recents().end()};
			return true;
		}
		return false;
	};

	auto path_sources = PathSource::List{};
	path_sources.sources.push_back(std::make_unique<DropFile>(events));
	path_sources.sources.push_back(std::make_unique<BrowseGltf>(events, config.config.file_menu.browse_path));
	path_sources.sources.push_back(std::make_unique<OpenRecent>(events));

	auto quit = Observer<event::Shutdown>{events, [&engine](event::Shutdown) { engine->request_stop(); }};
	auto browse_cd = Observer<event::BrowseCd>{events, [&config](event::BrowseCd const& cd) { config.config.file_menu.browse_path = cd.path; }};

	while (engine->running()) {
		auto const& state = engine->poll();
		auto const& input = state.input;
		auto const dt = state.dt;
		bool const mouse_look = input.mouse.held(GLFW_MOUSE_BUTTON_RIGHT);

		if (input.keyboard.pressed(GLFW_KEY_ESCAPE)) { engine->request_stop(); }
		glfwSetInputMode(engine->window(), GLFW_CURSOR, mouse_look ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
		if (!state.file_drops.empty()) { events->dispatch(event::FileDrop{state.file_drops.front()}); }

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

		if (auto menu = editor::MainMenu{}) {
			file_menu.display(*events, menu, {engine->load_status().stage > LoadStage::eNone});
			window_menu.display_menu(menu);
			window_menu.display_windows(*engine);

			if (auto path = path_sources.update(); !path.empty()) { load_async(std::move(path)); }
		}

		config.update(*engine);

		// TEMP CODE
		if (input.keyboard.pressed(GLFW_KEY_R)) {
			logger::info("Reloading...");
			engine.reset();
			init();
			continue;
		}

		if (auto* node = engine->scene().find(node_id)) {
			node->instances[0].rotate(glm::radians(drot_z[0]) * dt, {0.0f, 1.0f, 0.0f});
			node->instances[1].rotate(glm::radians(drot_z[1]) * dt, {1.0f, 0.0f, 0.0f});
		}
		// TEMP CODE

		engine->render();
	}
}

std::optional<std::uint32_t> to_u32(std::string const& s) {
	try {
		int i = std::stoi(s);
		if (i < 0) {
			logger::error("Invalid value: {}", i);
			return {};
		}
		return static_cast<std::uint32_t>(i);
	} catch (std::exception const& e) { logger::error("Invalid value: {} ({})", s, e.what()); }
	return {};
}
} // namespace

int main(int argc, char** argv) {
	try {
		auto logger_instance = logger::Instance{};
		struct Parser : CliOpts::Parser {
			AppOpts app_opts{};

			void opt(CliOpts::Key key, CliOpts::Value value) final {
				switch (key.single) {
				case 't': app_opts.force_threads = to_u32(std::string{value}); return;
				default: break;
				}
			}
		};
		auto spec = CliOpts::Spec{};
		spec.options = {
			CliOpts::Opt{
				.key = CliOpts::Key{.full = "force-threads", .single = 't'},
				.value = "COUNT",
				.is_optional_value = false,
				.help = "Force number of loading threads",
			},
		};
		spec.version = version_string();
		auto parser = Parser{};
		switch (CliOpts::parse(std::move(spec), &parser, argc, argv)) {
		case CliOpts::Result::eExitSuccess: return EXIT_SUCCESS;
		case CliOpts::Result::eExitFailure: return EXIT_FAILURE;
		case CliOpts::Result::eContinue: break;
		}
		try {
			run(parser.app_opts);
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
