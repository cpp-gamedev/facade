#pragma once
#include <memory>
#include <span>
#include <string>

namespace facade {
struct SpirV {
	struct View;

	std::unique_ptr<std::uint32_t[]> code{};
	std::size_t size{};

	static bool can_compile();
	static bool try_compile(SpirV& out_spirv, std::string& out_error, char const* glsl_path, char const* spir_v_path, bool debug);
	static SpirV compile(char const* glsl_path, char const* spir_v_path, bool debug);
	static SpirV compile(char const* glsl_path, bool debug) { return compile(glsl_path, "", debug); }

	View view() const;
	operator View() const;
	explicit operator bool() const { return !!code; }
};

struct SpirV::View {
	std::span<std::uint32_t const> code{};

	static View from_bytes(std::span<std::byte const> bytes);

	explicit operator bool() const { return !code.empty(); }
};
} // namespace facade
