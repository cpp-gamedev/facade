#include <facade/util/error.hpp>
#include <facade/util/mufo.hpp>
#include <facade/util/string.hpp>
#include <facade/vk/spir_v.hpp>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <utility>

namespace facade {
namespace fs = std::filesystem;

namespace {
constexpr std::string_view compiler_v = "glslc";
constexpr std::string_view dev_null_v =
#if defined(_WIN32)
	"NUL";
#else
	"/dev/null";
#endif

bool try_compiler() {
	auto const cmd = concat(compiler_v, " --version > ", dev_null_v);
	return std::system(cmd.c_str()) == 0;
}

bool compiler_available() {
	static std::atomic<bool> s_ret{};
	if (s_ret) { return s_ret; }
	return s_ret = try_compiler();
}

bool do_try_compile(char const* src, char const* dst, bool debug) {
	if (!compiler_available()) { return false; }
	if (!fs::is_regular_file(src)) { return false; }
	char const* debug_str = debug ? " -g " : "";
	auto const cmd = concat(compiler_v, debug_str, " ", src, " -o ", dst);
	return std::system(cmd.c_str()) == 0;
}

std::string get_spir_v_path(char const* glsl, char const* path) {
	if (path && *path) { return path; }
	return concat(glsl, ".spv");
}
} // namespace

bool SpirV::can_compile() { return compiler_available(); }

bool SpirV::try_compile(SpirV& out_spirv, std::string& out_error, char const* glsl_path, char const* spir_v_path, bool debug) {
	auto const dst = get_spir_v_path(glsl_path, spir_v_path);
	if (!do_try_compile(glsl_path, dst.c_str(), debug)) {
		out_error = concat("Failed to compile [", glsl_path, "]");
		return false;
	}
	auto file = std::ifstream(dst, std::ios::binary | std::ios::ate);
	if (!file) {
		out_error = concat("Failed to open [", dst, "]");
		return false;
	}
	auto const size = file.tellg();
	if (static_cast<std::size_t>(size) % sizeof(std::uint32_t) != 0) {
		out_error = concat("Invalid SPIR-V [", dst, "]");
		return false;
	}
	file.seekg({});
	out_spirv.size = static_cast<std::size_t>(size) / sizeof(std::uint32_t);
	out_spirv.code = make_unique_for_overwrite<std::uint32_t[]>(out_spirv.size);
	file.read(reinterpret_cast<char*>(out_spirv.code.get()), size);
	return true;
}

SpirV SpirV::compile(char const* glsl_path, char const* spir_v_path, bool debug) {
	auto const dst = get_spir_v_path(glsl_path, spir_v_path);
	auto ret = SpirV{};
	auto error = std::string{};
	if (!try_compile(ret, error, glsl_path, dst.c_str(), debug)) { throw Error{error}; }
	return ret;
}

auto SpirV::view() const -> View { return {.code = {code.get(), size}}; }
SpirV::operator View() const { return view(); }

auto SpirV::View::from_bytes(std::span<std::byte const> bytes) -> View {
	return {.code = {reinterpret_cast<std::uint32_t const*>(bytes.data()), bytes.size() / sizeof(std::uint32_t)}};
}
} // namespace facade
