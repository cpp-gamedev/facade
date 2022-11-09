#include <fmt/format.h>
#include <facade/util/cmd_args.hpp>
#include <facade/util/visitor.hpp>
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <span>
#include <variant>

namespace facade {
namespace {
namespace fs = std::filesystem;

CmdArgs::Opt const* find_opt(CmdArgs::Spec const& spec, std::variant<std::string_view, char> key) {
	for (auto const& opt : spec.options) {
		bool match{};
		auto const visitor = Visitor{
			[opt, &match](std::string_view const full) {
				if (opt.key.full == full) { match = true; }
			},
			[opt, &match](char const single) {
				if (opt.key.single == single) { match = true; }
			},
		};
		std::visit(visitor, key);
		if (match) { return &opt; }
	}
	return {};
}

struct OptParser {
	using Result = CmdArgs::Result;

	CmdArgs::Spec spec{};
	Ptr<CmdArgs::Parser> out;
	std::string exe_name{};

	OptParser(CmdArgs::Spec spec, Ptr<CmdArgs::Parser> out, char const* arg0) : spec(std::move(spec)), out(out) {
		std::erase_if(this->spec.options, [](CmdArgs::Opt const& o) { return !o.key.valid(); });
		this->spec.options.push_back(CmdArgs::Opt{.key = {.full = "help"}, .help = "Show this help text"});
		this->spec.options.push_back(CmdArgs::Opt{.key = {.full = "version"}, .help = "Show the version"});
		exe_name = fs::path{arg0}.filename().stem().generic_string();
	}

	std::size_t get_max_width() const {
		auto ret = std::size_t{};
		for (auto const& opt : spec.options) {
			auto width = opt.key.full.size();
			if (!opt.value.empty()) {
				width += 1; // =
				if (opt.is_optional_value) {
					width += 2; // []
				}
				width += opt.value.size();
			}
			ret = std::max(ret, width);
		}
		return ret;
	}

	Result print_help() const {
		auto str = std::string{};
		str.reserve(1024);
		fmt::format_to(std::back_inserter(str), "Usage: {} [OPTION]...\n\n", exe_name);
		auto const max_width = get_max_width();
		for (auto const& opt : spec.options) {
			fmt::format_to(std::back_inserter(str), "  {}{}", (opt.key.single ? '-' : ' '), (opt.key.single ? opt.key.single : ' '));
			if (!opt.key.full.empty()) { fmt::format_to(std::back_inserter(str), "{} --{}", (opt.key.single ? ',' : ' '), opt.key.full); }
			auto width = opt.key.full.size();
			if (!opt.value.empty()) {
				fmt::format_to(std::back_inserter(str), "{}={}{}", (opt.is_optional_value ? "[" : ""), opt.value, (opt.is_optional_value ? "]" : ""));
				width += (opt.is_optional_value ? 3 : 1) + opt.value.size();
			}
			assert(width <= max_width);
			auto const remain = max_width - width + 4;
			for (std::size_t i = 0; i < remain; ++i) { str += ' '; }
			fmt::format_to(std::back_inserter(str), "{}\n", opt.help);
		}
		std::cout << str << '\n';
		return Result::eExitSuccess;
	}

	Result print_version() const {
		std::cout << fmt::format("{} version {}\n", exe_name, spec.version);
		return Result::eExitSuccess;
	}

	CmdArgs::Result opt(CmdArgs::Key key, CmdArgs::Value value) const {
		if (key.full == "help") { return print_help(); }
		if (key.full == "version") { return print_version(); }
		if (out) { out->opt(key, value); }
		return Result::eContinue;
	}
};

struct ParseOpt {
	using Result = CmdArgs::Result;

	OptParser const& out;

	std::string_view current{};

	bool at_end() const { return current.empty(); }

	char advance() {
		if (at_end()) { return {}; }
		auto ret = current[0];
		current = current.substr(1);
		return ret;
	}

	char peek() const { return current[0]; }

	Result unknown_option(std::string_view opt) const {
		std::cerr << fmt::format("unknown option: {}\n", opt);
		return Result::eExitFailure;
	}

	Result missing_value(CmdArgs::Opt const& opt) const {
		auto str = opt.key.full;
		if (str.empty()) { str = {&opt.key.single, 1}; }
		std::cerr << fmt::format("missing required value for option: {}{}\n", (opt.key.full.empty() ? "-" : "--"), str);
		return Result::eExitFailure;
	}

	Result parse_word() {
		auto const it = current.find('=');
		CmdArgs::Opt const* opt{};
		auto value = CmdArgs::Value{};
		if (it != std::string_view::npos) {
			auto const key = current.substr(0, it);
			opt = find_opt(out.spec, key);
			if (!opt) { return unknown_option(key); }
			value = current.substr(it + 1);
		} else {
			opt = find_opt(out.spec, current);
			if (!opt) { return unknown_option(current); }
		}
		if (!opt->value.empty() && !opt->is_optional_value && value.empty()) { return missing_value(*opt); }
		return out.opt(opt->key, value);
	}

	Result parse_singles() {
		char prev{};
		while (!at_end() && peek() != '=') {
			prev = advance();
			auto const* opt = find_opt(out.spec, prev);
			if (!opt) { return unknown_option({&prev, 1}); }
			return out.opt(opt->key, {});
		}
		if (peek() == '=') {
			if (prev == '\0') { return unknown_option(""); }
			advance();
			auto const* opt = find_opt(out.spec, prev);
			if (!opt) { return unknown_option({&prev, 1}); }
			return out.opt(opt->key, current);
		}
		return Result::eContinue;
	}

	Result parse() {
		auto const c = advance();
		if (c == '-') {
			advance();
			return parse_word();
		}
		return parse_singles();
	}

	Result operator()(std::string_view opt_arg) {
		current = opt_arg;
		return parse();
	}
};

} // namespace

auto CmdArgs::parse(Spec spec, Ptr<Parser> out, int argc, char const* const* argv) -> Result {
	if (argc < 1) { return Result::eContinue; }
	auto opt_parser = OptParser{std::move(spec), out, argv[0]};
	auto parse_opt = ParseOpt{opt_parser};
	for (int i = 1; i < argc; ++i) {
		if (auto const result = parse_opt(argv[i]); result != Result::eContinue) { return result; }
	}
	return Result::eContinue;
}

auto CmdArgs::parse(std::string_view version, int argc, char const* const* argv) -> Result {
	auto spec = Spec{.version = version};
	return parse(std::move(spec), {}, argc, argv);
}
} // namespace facade
