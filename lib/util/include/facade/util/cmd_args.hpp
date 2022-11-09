#pragma once
#include <facade/util/ptr.hpp>
#include <string>
#include <vector>

namespace facade {
struct CmdArgs {
	enum class Result { eContinue, eExitFailure, eExitSuccess };

	struct Key {
		std::string_view full{};
		char single{};

		constexpr bool valid() const { return !full.empty() || single != '\0'; }
	};

	using Value = std::string_view;

	struct Opt {
		Key key{};
		Value value{};
		bool is_optional_value{};
		std::string_view help{};
	};

	struct Parser {
		virtual void opt(Key key, Value value) = 0;
	};

	struct Spec {
		std::vector<Opt> options{};
		std::string_view version{"(unknown)"};
	};

	static Result parse(Spec spec, Ptr<Parser> out, int argc, char const* const* argv);
	static Result parse(std::string_view version, int argc, char const* const* argv);
};
} // namespace facade
