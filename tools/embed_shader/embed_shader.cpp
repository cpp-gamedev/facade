#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
namespace fs = std::filesystem;

bool is_alpha(char const c) { return std::isalpha(static_cast<unsigned char>(c)); }
bool is_digit(char const c) { return std::isdigit(static_cast<unsigned char>(c)); }

constexpr bool validate_identifier(std::string_view const ident) {
	if (ident[0] != '_' && !is_alpha(ident[0])) { return false; }
	return std::all_of(ident.begin() + 1, ident.end(), [](char const c) { return c == '_' || is_alpha(c) || is_digit(c); });
}

void write(std::ifstream& in_file, std::string_view const ident) {
	auto col = std::size_t{};
	auto indent = int{1};
	auto line_feed = [&indent, &col] {
		std::cout << '\n';
		for (int i = 0; i < indent; ++i) { std::cout << '\t'; }
		col = 0;
	};

	std::cout << "inline constexpr unsigned char " << ident << "_v[] = {";
	indent = 1;
	line_feed();
	char c;
	while (in_file.get(c)) {
		auto str = std::stringstream{};
		str << "0x" << std::hex << static_cast<std::uint32_t>(static_cast<unsigned char>(c)) << ", ";
		auto out = str.str();
		col += out.size();
		std::cout << out;
		if (++col >= 160) { line_feed(); }
	}
	indent = 0;
	line_feed();
	std::cout << "};\n";
}

template <typename... T>
std::string concat(T const&... t) {
	auto ret = std::stringstream{};
	((ret << t), ...);
	return ret.str();
}

bool compile(fs::path const& glsl, fs::path& out_spir_v) {
	out_spir_v = glsl;
	out_spir_v += ".spv";
	return std::system(concat("glslc ", glsl.generic_string(), " -o ", out_spir_v.generic_string()).c_str()) == 0;
}

void open(char const* const file, std::string_view const ident) {
	struct {
		fs::path glsl{};
		fs::path spir_v{};
		std::ifstream file{};
	} entry{.glsl = file};
	if (!fs::is_regular_file(entry.glsl)) { throw std::runtime_error{"invalid GLSL path: " + entry.glsl.generic_string()}; }
	if (!compile(entry.glsl, entry.spir_v)) { throw std::runtime_error{"glslc failed to compile GLSL to SPIR-V: " + entry.glsl.generic_string()}; }
	auto test = std::ifstream{entry.spir_v, std::ios::binary | std::ios::app};
	if (!entry.file) { throw std::runtime_error{"failed to open SPIR-V: " + entry.spir_v.generic_string()}; }
	entry.file = std::ifstream{entry.spir_v, std::ios::binary};

	std::cout << R"(#pragma once

namespace facade {
)";
	write(entry.file, ident);
	std::cout << R"(} // namespace facade
)";
}

std::string ident_from_filename(std::string_view prefix) {
	auto it = prefix.find_last_of('/');
	if (it == std::string_view::npos) { it = prefix.find_last_of('\\'); }
	if (it != std::string_view::npos) { prefix = prefix.substr(it + 1); }
	auto ret = std::string{prefix};
	for (char& c : ret) {
		if (c == '.') { c = '_'; }
	}
	return ret;
}

void run(int argc, char const* const argv[]) {
	if (argc < 1) { throw std::runtime_error{"no arguments passed"}; }
	auto const exe = fs::path{argv[0]}.filename().generic_string();
	auto print_usage = [exe] { std::cerr << "Usage: " << exe << " <shader> [identifier]\n"; };
	if (argc < 2) {
		print_usage();
		throw std::runtime_error{"insufficient arguments passed"};
	}
	auto const ident = argc > 2 ? std::string{argv[2]} : ident_from_filename(argv[1]);
	if (!validate_identifier(ident)) { throw std::runtime_error{"invalid identifier: " + std::string{ident}}; }

	open(argv[1], ident.data());
}
} // namespace

// Syntax: embed-shader <shader> <identifier>
// 	shader: path to GLSL source, eg `shaders/lit.vert` or `shaders/lit.frag`
// 	identifier: variable name in generated code, eg `lit_frag` for `lit_frag_v`
int main(int argc, char** argv) {
	try {
		run(argc, argv);
	} catch (std::exception const& e) {
		std::cerr << "Error: " << e.what() << '\n';
		return EXIT_FAILURE;
	}
}
