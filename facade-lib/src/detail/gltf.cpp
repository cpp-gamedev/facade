#include <detail/gltf.hpp>
#include <djson/json.hpp>
#include <facade/util/data_provider.hpp>
#include <facade/util/error.hpp>
#include <facade/util/geometry.hpp>
#include <algorithm>
#include <bit>
#include <cstring>

namespace facade {
namespace gltf {
namespace {
constexpr std::size_t get_base64_start(std::string_view str) {
	constexpr auto match_v = std::string_view{";base64,"};
	auto const it = str.find(match_v);
	if (it == std::string_view::npos) { return it; }
	return it + match_v.size();
}

ByteBuffer base64_decode(std::string_view const base64) {
	static constexpr unsigned char table[] = {
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64, 64, 0,	1,	2,	3,	4,	5,	6,	7,	8,
		9,	10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64, 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64};

	std::size_t const in_len = base64.size();
	assert(in_len % 4 == 0);
	if (in_len % 4 != 0) { return {}; }

	std::size_t out_len = in_len / 4 * 3;
	if (base64[in_len - 1] == '=') out_len--;
	if (base64[in_len - 2] == '=') out_len--;

	auto ret = ByteBuffer::make(out_len);

	for (std::uint32_t i = 0, j = 0; i < in_len;) {
		std::uint32_t const a = base64[i] == '=' ? 0 & i++ : table[static_cast<std::size_t>(base64[i++])];
		std::uint32_t const b = base64[i] == '=' ? 0 & i++ : table[static_cast<std::size_t>(base64[i++])];
		std::uint32_t const c = base64[i] == '=' ? 0 & i++ : table[static_cast<std::size_t>(base64[i++])];
		std::uint32_t const d = base64[i] == '=' ? 0 & i++ : table[static_cast<std::size_t>(base64[i++])];

		std::uint32_t const triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + (d << 0 * 6);

		if (j < out_len) ret.data()[j++] = static_cast<std::byte>((triple >> 2 * 8) & 0xFF);
		if (j < out_len) ret.data()[j++] = static_cast<std::byte>((triple >> 1 * 8) & 0xFF);
		if (j < out_len) ret.data()[j++] = static_cast<std::byte>((triple >> 0 * 8) & 0xFF);
	}

	return ret;
}

template <typename T>
T from_bytes(std::span<std::byte const>& out_bytes) {
	assert(out_bytes.size() >= sizeof(T));
	T ret;
	if constexpr (std::endian::native == std::endian::big) {
		auto const rspan = out_bytes.subspan(sizeof(T));
		std::byte buffer[sizeof(T)]{};
		std::reverse_copy(rspan.begin(), rspan.end(), buffer);
		std::memcpy(&ret, buffer, sizeof(T));
	} else {
		std::memcpy(&ret, out_bytes.data(), sizeof(T));
	}
	out_bytes = out_bytes.subspan(sizeof(T));
	return ret;
}

template <typename T>
std::vector<T> vec_from_bytes(std::span<std::byte const> bytes, std::size_t count) {
	assert(count * sizeof(T) <= bytes.size());
	auto ret = std::vector<T>{};
	ret.reserve(count);
	for (std::size_t i = 0; i < count; ++i) { ret.push_back(from_bytes<T>(bytes)); }
	return ret;
}

template <typename T, glm::length_t Dim>
void push_converted(std::vector<glm::vec<Dim, float>>& out, glm::vec<Dim, T> const& other_vec) {
	out.push_back(glm::vec<Dim, float>{other_vec});
}

template <typename To, typename From>
void push_converted(std::vector<To>& out, From const& other) {
	out.push_back(static_cast<To>(other));
}

template <typename To, typename From>
std::vector<To> convert_vec(From other_vec) {
	if constexpr (std::same_as<typename From::value_type, To>) {
		return other_vec;
	} else {
		auto ret = std::vector<To>{};
		ret.reserve(other_vec.size());
		for (auto const& t : other_vec) { push_converted(ret, t); }
		return ret;
	}
}

enum class Target { eArrayBuffer = 34962, eElementArrayBuffer = 34693 };

struct Span {
	std::size_t length{};
	std::size_t offset{};
};

struct Buffer {
	std::string_view name{};
	ByteBuffer bytes{};
};

template <typename T>
struct Range {
	T min{};
	T max{};
};

struct BufferView {
	std::size_t buffer{};
	Span span{};
	Target target{};
};

struct Accessor {
	enum class ComponentType { eByte = 5120, eUnsignedByte = 5121, eShort = 5122, eUnsignedShort = 5123, eUnsignedInt = 5125, eFloat = 5126 };
	enum class Type { eScalar, eVec2, eVec3, eVec4, eMat2, eMat3, eMat4, eCOUNT_ };
	static constexpr std::string_view type_str_v[] = {"SCALAR", "VEC2", "VEC3", "VEC4", "MAT2", "MAT3", "MAT4"};
	static_assert(std::size(type_str_v) == static_cast<std::size_t>(Type::eCOUNT_));

	static constexpr bool is_vec_type(Type type, std::size_t dim) {
		switch (type) {
		case Type::eVec2: return dim == 2;
		case Type::eVec3: return dim == 3;
		case Type::eVec4: return dim == 4;
		default: return false;
		}
	}

	static Type get_type(std::string_view const key) {
		for (int i = 0; i < static_cast<int>(Type::eCOUNT_); ++i) {
			if (type_str_v[i] == key) { return static_cast<Type>(i); }
		}
		// TODO: better error
		throw Error{"todo: gltf error"};
	}

	std::string_view name{};
	std::optional<std::size_t> buffer_view{};
	std::size_t byte_offset{};
	ComponentType component_type{};
	bool normalized{false};
	std::size_t count{};
	Type type{};
	Range<std::array<std::optional<float>, 16>> ranges{};
};

struct Attributes {
	std::optional<std::size_t> position{};
	std::optional<std::size_t> normal{};
	std::optional<std::size_t> tangent{};
	std::vector<std::size_t> tex_coords{};
	std::vector<std::size_t> colours{};
	std::vector<std::size_t> joints{};
	std::vector<std::size_t> weights{};

	static std::optional<std::size_t> get_if(dj::Json const& value) {
		if (value.is_number()) { return value.as<std::size_t>(); }
		return {};
	}

	static std::vector<std::size_t> fill(dj::Json const& parent, std::string_view const prefix, std::size_t max = 16) {
		auto ret = std::vector<std::size_t>{};
		for (std::size_t i = 0; i < max; ++i) {
			auto const str = std::string{prefix} + "_" + std::to_string(i);
			auto const& json = parent[str];
			if (!json) { return ret; }
			ret.push_back(json.as<std::size_t>());
		}
		return ret;
	}

	static Attributes parse(dj::Json const& json) {
		auto ret = Attributes{};
		ret.position = get_if(json["POSITION"]);
		ret.normal = get_if(json["NORMAL"]);
		ret.tangent = get_if(json["TANGENT"]);
		ret.tex_coords = fill(json, "TEXCOORD");
		ret.colours = fill(json, "COLOR");
		ret.joints = fill(json, "JOINTS");
		ret.weights = fill(json, "WEIGHTS");
		return ret;
	}
};

struct MeshData {
	struct Primitive {
		Attributes attributes{};
		std::optional<std::size_t> indices{};
		std::optional<std::size_t> material{};
	};

	std::string name{};
	std::vector<Primitive> primitives{};
	std::optional<std::size_t> material{};
};

struct Data {
	std::span<Buffer const> buffers;
	std::span<BufferView const> buffer_views;
	std::span<Accessor const> accessors;
	std::span<Material const> materials;
	std::span<MeshData const> meshes;

	std::span<std::byte const> view_buffer(std::size_t buffer_view) const {
		assert(buffer_view < buffer_views.size());
		auto const& bv = buffer_views[buffer_view];
		assert(bv.buffer < buffers.size());
		auto const& buffer = buffers[bv.buffer];
		return buffer.bytes.span().subspan(bv.span.offset, bv.span.length);
	}

	template <typename T>
	std::vector<T> as_t(std::optional<std::size_t> const accessor) const {
		if (!accessor) { return {}; }
		assert(*accessor < accessors.size());
		auto const& a = accessors[*accessor];
		assert(a.type == Accessor::Type::eScalar);
		if (!a.buffer_view) { return std::vector<T>(a.count); }
		auto const v = view_buffer(*a.buffer_view).subspan(a.byte_offset);
		switch (a.component_type) {
		case Accessor::ComponentType::eByte: return convert_vec<T>(vec_from_bytes<std::int8_t>(v, a.count));
		case Accessor::ComponentType::eUnsignedByte: return convert_vec<T>(vec_from_bytes<std::uint8_t>(v, a.count));
		case Accessor::ComponentType::eShort: return convert_vec<T>(vec_from_bytes<std::int16_t>(v, a.count));
		case Accessor::ComponentType::eUnsignedShort: return convert_vec<T>(vec_from_bytes<std::uint16_t>(v, a.count));
		case Accessor::ComponentType::eUnsignedInt: return convert_vec<T>(vec_from_bytes<std::uint32_t>(v, a.count));
		case Accessor::ComponentType::eFloat: return convert_vec<T>(vec_from_bytes<float>(v, a.count));
		}
		return {};
	}

	template <typename T, glm::length_t Dim>
	std::vector<glm::vec<Dim, T>> as_vec(std::optional<std::size_t> const accessor) const {
		if (!accessor) { return {}; }
		assert(*accessor < accessors.size());
		auto const& a = accessors[*accessor];
		assert(Accessor::is_vec_type(a.type, Dim));
		if (!a.buffer_view) { return std::vector<glm::vec<Dim, T>>(a.count); }
		auto const v = view_buffer(*a.buffer_view).subspan(a.byte_offset);
		switch (a.component_type) {
		case Accessor::ComponentType::eByte: return convert_vec<glm::vec<Dim, T>>(vec_from_bytes<glm::vec<Dim, std::int8_t>>(v, a.count));
		case Accessor::ComponentType::eUnsignedByte: return convert_vec<glm::vec<Dim, T>>(vec_from_bytes<glm::vec<Dim, std::uint8_t>>(v, a.count));
		case Accessor::ComponentType::eShort: return convert_vec<glm::vec<Dim, T>>(vec_from_bytes<glm::vec<Dim, std::int16_t>>(v, a.count));
		case Accessor::ComponentType::eUnsignedShort: return convert_vec<glm::vec<Dim, T>>(vec_from_bytes<glm::vec<Dim, std::uint16_t>>(v, a.count));
		case Accessor::ComponentType::eUnsignedInt: return convert_vec<glm::vec<Dim, T>>(vec_from_bytes<glm::vec<Dim, std::uint32_t>>(v, a.count));
		case Accessor::ComponentType::eFloat: return convert_vec<glm::vec<Dim, T>>(vec_from_bytes<glm::vec<Dim, float>>(v, a.count));
		}
		return {};
	}

	struct Storage {
		std::vector<Buffer> buffers{};
		std::vector<BufferView> buffer_views{};
		std::vector<Accessor> accessors{};
		std::vector<Camera> cameras{};
		std::vector<Sampler> samplers{};
		std::vector<Image> images{};
		std::vector<Texture> textures{};
		std::vector<Material> materials{};
		std::vector<MeshData> meshes{};

		Data data() const { return {buffers, buffer_views, accessors, materials, meshes}; }
	};

	static bool check(ByteBuffer const& a, ByteBuffer const& b) {
		if (a.size != b.size) { return false; }
		for (std::size_t i = 0; i < a.size; ++i) {
			if (a.bytes.get()[i] != b.bytes.get()[i]) { return false; }
		}
		return true;
	}

	struct Parser {
		DataProvider const& provider;
		Storage storage{};

		void buffer(dj::Json const& json) {
			auto& b = storage.buffers.emplace_back();
			b.name = json["name"].as_string();
			auto const uri = json["uri"].as_string();
			// TODO
			assert(!uri.empty());
			if (auto i = get_base64_start(uri); i != std::string_view::npos) {
				b.bytes = base64_decode(uri.substr(i));
			} else {
				b.bytes = provider.load(uri);
			}
		}

		void buffer_view(dj::Json const& json) {
			auto& bv = storage.buffer_views.emplace_back();
			// TODO
			assert(json.contains("buffer") && json.contains("byteLength"));
			bv.buffer = json["buffer"].as<std::size_t>();
			bv.span = {.length = json["byteLength"].as<std::size_t>(), .offset = json["byteOffset"].as<std::size_t>(0U)};
			bv.target = static_cast<Target>(json["target"].as<int>());
		}

		void accessor(dj::Json const& json) {
			auto& a = storage.accessors.emplace_back();
			// TODO
			assert(json.contains("componentType") && json.contains("count") && json.contains("type"));
			a.component_type = static_cast<Accessor::ComponentType>(json["componentType"].as<int>());
			a.count = json["count"].as<std::size_t>();
			a.type = Accessor::get_type(json["type"].as_string());

			if (auto const& bv = json["bufferView"]) { a.buffer_view = bv.as<std::size_t>(); }
			a.byte_offset = json["byteOffset"].as<std::size_t>(0U);
			a.normalized = json["normalized"].as_bool(dj::Boolean{false}).value;
			a.name = json["name"].as_string();
			// TODO range
		}

		Camera::Orthographic orthographic(dj::Json const& json) const {
			assert(json.contains("xmag") && json.contains("ymag") && json.contains("zfar") && json.contains("znear"));
			auto ret = Camera::Orthographic{};
			ret.xmag = json["xmag"].as<float>();
			ret.ymag = json["ymag"].as<float>();
			ret.zfar = json["zfar"].as<float>();
			ret.znear = json["znear"].as<float>();
			return ret;
		}

		Camera::Perspective perspective(dj::Json const& json) const {
			assert(json.contains("yfov") && json.contains("znear"));
			auto ret = Camera::Perspective{};
			ret.yfov = json["yfov"].as<float>();
			ret.znear = json["znear"].as<float>();
			ret.aspect_ratio = json["aspectRatio"].as<float>(0.0f);
			if (auto zfar = json["zfar"]) { ret.zfar = zfar.as<float>(); }
			return ret;
		}

		void camera(dj::Json const& json) {
			auto& c = storage.cameras.emplace_back();
			assert(json.contains("type"));
			c.name = json["name"].as_string();
			c.type = json["type"].as_string() == "orthographic" ? Camera::Type::eOrthographic : Camera::Type::ePerspective;
			if (c.type == Camera::Type::eOrthographic) {
				assert(json.contains("orthographic"));
				c.orthographic = orthographic(json["orthographic"]);
			} else {
				assert(json.contains("perspective"));
				c.perspective = perspective(json["perspective"]);
			}
		}

		void sampler(dj::Json const& json) {
			auto& s = storage.samplers.emplace_back();
			s.name = json["name"].as<std::string>();
			if (auto const& min = json["minFilter"]) { s.min_filter = static_cast<Filter>(min.as<int>()); }
			if (auto const& mag = json["magFilter"]) { s.mag_filter = static_cast<Filter>(mag.as<int>()); }
			s.wrap_s = static_cast<Wrap>(json["wrapS"].as<int>(static_cast<int>(s.wrap_s)));
			s.wrap_t = static_cast<Wrap>(json["wrapT"].as<int>(static_cast<int>(s.wrap_t)));
		}

		MeshData::Primitive primitive(dj::Json const& json) const {
			auto ret = MeshData::Primitive{};
			assert(json.contains("attributes"));
			ret.attributes = Attributes::parse(json["attributes"]);
			if (auto const& indices = json["indices"]) { ret.indices = indices.as<std::size_t>(); }
			if (auto const& material = json["material"]) { ret.material = material.as<std::size_t>(); }
			return ret;
		}

		void mesh(dj::Json const& json) {
			auto& m = storage.meshes.emplace_back();
			m.name = json["name"].as_string();
			for (auto const& j : json["primitives"].array_view()) { m.primitives.push_back(primitive(j)); }
		}

		void image(dj::Json const& json) {
			auto& i = storage.images.emplace_back();
			auto const uri = json["uri"].as_string();
			auto name = std::string{json["name"].as_string()};
			if (auto const it = get_base64_start(uri); it != std::string_view::npos) {
				i = Image{base64_decode(uri.substr(it)).span(), std::move(name)};
			} else {
				i = Image{provider.load(uri).span(), std::move(name)};
			}
		}

		void texture(dj::Json const& json) {
			auto& t = storage.textures.emplace_back();
			t.name = json["name"].as<std::string>();
			if (auto const& sampler = json["sampler"]) { t.sampler = sampler.as<std::size_t>(); }
			assert(json.contains("source"));
			t.source = json["source"].as<std::size_t>();
		}

		template <typename T, glm::length_t Dim>
		static glm::vec<Dim, T> get_vec(dj::Json const& value, glm::vec<Dim, T> const& fallback = {}) {
			auto ret = fallback;
			if (!value) { return ret; }
			assert(value.array_view().size() >= Dim);
			ret.x = value[0].as<T>();
			if constexpr (Dim > 1) { ret.y = value[1].as<T>(); }
			if constexpr (Dim > 2) { ret.z = value[2].as<T>(); }
			if constexpr (Dim > 3) { ret.w = value[3].as<T>(); }
			return ret;
		}

		static constexpr Material::AlphaMode get_alpha_mode(std::string_view const mode) {
			if (mode == "MASK") {
				return Material::AlphaMode::eMask;
			} else if (mode == "BLEND") {
				return Material::AlphaMode::eBlend;
			}
			return Material::AlphaMode::eOpaque;
		}

		static TextureInfo get_texture_info(dj::Json const& json) {
			assert(json.contains("index"));
			auto ret = TextureInfo{};
			ret.texture = json["index"].as<std::size_t>();
			ret.tex_coord = json["texCoord"].as<std::size_t>(ret.tex_coord);
			return ret;
		}

		static NormalTextureInfo get_normal_texture_info(dj::Json const& json) {
			auto ret = NormalTextureInfo{};
			ret.info = get_texture_info(json);
			ret.scale = json["scale"].as<float>(ret.scale);
			return ret;
		}

		static OccusionTextureInfo get_occlusion_texture_info(dj::Json const& json) {
			auto ret = OccusionTextureInfo{};
			ret.info = get_texture_info(json);
			ret.strength = json["strength"].as<float>(ret.strength);
			return ret;
		}

		static PbrMetallicRoughness pbr_metallic_roughness(dj::Json const& json) {
			auto ret = PbrMetallicRoughness{};
			ret.base_colour_factor = get_vec<float, 4>(json["baseColorFactor"], ret.base_colour_factor);
			if (auto const& bct = json["baseColorTexture"]) { ret.base_colour_texture = get_texture_info(bct); }
			ret.metallic_factor = json["metallicFactor"].as<float>(ret.metallic_factor);
			ret.roughness_factor = json["roughnessFactor"].as<float>(ret.roughness_factor);
			if (auto const& mrt = json["metallicRoughnessTexture"]) { ret.metallic_roughness_texture = get_texture_info(mrt); }
			return ret;
		}

		void material(dj::Json const& json) {
			auto& m = storage.materials.emplace_back();
			m.name = std::string{json["name"].as_string()};
			m.pbr = pbr_metallic_roughness(json["pbrMetallicRoughness"]);
			m.emissive_factor = get_vec<float, 3>(json["emissiveFactor"]);
			if (auto const& nt = json["normalTexture"]) { m.normal_texture = get_normal_texture_info(nt); }
			if (auto const& ot = json["occlusionTexture"]) { m.occlusion_texture = get_occlusion_texture_info(ot); }
			if (auto const& et = json["emissiveTexture"]) { m.emissive_texture = get_texture_info(et); }
			m.alpha_mode = get_alpha_mode(json["alphaMode"].as_string());
			m.alpha_cutoff = json["alphaCutoff"].as<float>(m.alpha_cutoff);
			m.double_sided = json["doubleSided"].as_bool(dj::Boolean{m.double_sided}).value;
		}

		Storage parse(dj::Json const& scene) {
			storage = {};
			for (auto const& b : scene["buffers"].array_view()) { buffer(b); }
			for (auto const& bv : scene["bufferViews"].array_view()) { buffer_view(bv); }
			for (auto const& s : scene["accessors"].array_view()) { accessor(s); }

			for (auto const& c : scene["cameras"].array_view()) { camera(c); }
			for (auto const& s : scene["samplers"].array_view()) { sampler(s); }
			for (auto const& i : scene["images"].array_view()) { image(i); }
			for (auto const& t : scene["textures"].array_view()) { texture(t); }
			for (auto const& m : scene["materials"].array_view()) { material(m); }

			for (auto const& m : scene["meshes"].array_view()) { mesh(m); }

			// Texture will use ColourSpace::sRGB by default; change non-colour textures to be linear
			auto set_linear = [this](std::size_t index) { storage.textures.at(index).colour_space = ColourSpace::eLinear; };

			// Determine whether a texture points to colour data by referring to the material(s) it is used in
			// In our case all material textures except pbr.base_colour_texture are linear
			// The GLTF spec mandates image formats for corresponding material textures:
			// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-material
			for (auto const& m : storage.materials) {
				if (m.pbr.metallic_roughness_texture) { set_linear(m.pbr.metallic_roughness_texture->texture); }
				if (m.emissive_texture) { set_linear(m.emissive_texture->texture); }
				if (m.occlusion_texture) { set_linear(m.occlusion_texture->info.texture); }
				if (m.normal_texture) { set_linear(m.normal_texture->info.texture); }
			}

			return std::move(storage);
		}
	};
};

template <typename Ret, std::size_t... I>
Ret to(std::index_sequence<I...>, dj::Json const& json) {
	assert(json.array_view().size() >= sizeof...(I));
	return Ret{json[I].as<float>()...};
}

template <typename>
[[maybe_unused]] constexpr bool false_v{false};

template <typename Ret>
Ret to(dj::Json const& json) {
	if constexpr (std::same_as<Ret, glm::mat4x4>) {
		return to<glm::mat4>(std::make_index_sequence<16>(), json);
	} else if constexpr (std::same_as<Ret, glm::quat> || std::same_as<Ret, glm::vec4>) {
		return to<Ret>(std::make_index_sequence<4>(), json);
	} else if constexpr (std::same_as<Ret, glm::vec3>) {
		return to<glm::vec3>(std::make_index_sequence<3>(), json);
	} else {
		static_assert(false_v<Ret>, "Invalid type");
	}
}
} // namespace

namespace {
std::vector<Vertex> build_vertices(std::span<glm::vec3 const> p, std::span<glm::vec3 const> rgb, std::span<glm::vec3 const> n, std::span<glm::vec2 const> uv) {
	if (p.empty()) { return {}; }
	assert(rgb.empty() || rgb.size() == p.size());
	assert(n.empty() || n.size() == p.size());
	assert(uv.empty() || uv.size() == p.size());
	auto ret = std::vector<Vertex>(p.size());
	for (std::size_t i = 0; i < ret.size(); ++i) {
		ret[i].position = p[i];
		if (!rgb.empty()) { ret[i].rgb = rgb[i]; }
		if (!n.empty()) { ret[i].normal = n[i]; }
		if (!uv.empty()) { ret[i].uv = uv[i]; }
	}
	return ret;
}

Geometry build_geometry(Data const data, MeshData::Primitive const& mp) {
	auto ret = Geometry{};
	if (!mp.attributes.position) { return {}; }
	auto const positions = data.as_vec<float, 3>(*mp.attributes.position);
	auto const normals = data.as_vec<float, 3>(mp.attributes.normal);
	auto const rgb = mp.attributes.colours.empty() ? std::vector<glm::vec3>{} : data.as_vec<float, 3>(mp.attributes.colours.front());
	auto const uv = mp.attributes.tex_coords.empty() ? std::vector<glm::vec2>{} : data.as_vec<float, 2>(mp.attributes.tex_coords.front());
	ret.vertices = build_vertices(positions, rgb, normals, uv);
	if (mp.indices) { ret.indices = data.as_t<std::uint32_t>(*mp.indices); }
	return ret;
}

Transform transform(dj::Json const& node) {
	auto ret = Transform{};
	if (auto const& mat = node["matrix"]) {
		[[maybe_unused]] auto const& arr = mat.array_view();
		assert(arr.size() == 16);
		ret.set_matrix(to<glm::mat4x4>(mat));
	} else {
		if (auto const& rotation = node["rotation"]) { ret.set_orientation(to<glm::quat>(rotation)); }
		if (auto const& scale = node["scale"]) { ret.set_scale(to<glm::vec3>(scale)); }
		if (auto const& translation = node["translation"]) { ret.set_position(to<glm::vec3>(translation)); }
	}
	return ret;
}

std::vector<std::size_t> children(dj::Json const& json) {
	auto ret = std::vector<std::size_t>{};
	auto const& children = json.array_view();
	ret.reserve(children.size());
	for (auto const& index : children) { ret.push_back(index.as<std::size_t>()); }
	return ret;
}
} // namespace

Asset Asset::parse(dj::Json const& json, DataProvider const& provider) {
	auto ret = Asset{};
	auto storage = Data::Parser{provider}.parse(json);
	if (storage.accessors.empty()) { return {}; }
	ret.cameras = std::move(storage.cameras);
	ret.images = std::move(storage.images);
	ret.materials = std::move(storage.materials);
	ret.samplers = std::move(storage.samplers);
	ret.textures = std::move(storage.textures);

	auto const data = storage.data();
	for (auto const& mesh_data : data.meshes) {
		auto& mesh = ret.meshes.emplace_back();
		mesh.name = mesh_data.name;
		for (auto const& primitive : mesh_data.primitives) {
			mesh.primitives.push_back(Mesh::Primitive{ret.geometries.size(), primitive.material});
			ret.geometries.push_back(build_geometry(data, primitive));
		}
	}

	auto const& nodes = json["nodes"].array_view();
	ret.nodes.reserve(nodes.size());
	for (auto const& node : nodes) {
		auto type = Node::Type::eNone;
		auto index = std::size_t{};
		auto const& mesh = node["mesh"];
		auto const& camera = node["camera"];
		if (mesh || camera) {
			assert(mesh.is_number() ^ camera.is_number());
			type = camera ? Node::Type::eCamera : Node::Type::eMesh;
			index = camera ? camera.as<std::size_t>() : mesh.as<std::size_t>();
		}
		ret.nodes.push_back(Node{node["name"].as<std::string>(), transform(node), children(node["children"]), index, type});
	}

	auto const& scenes = json["scenes"].array_view();
	ret.scenes.reserve(scenes.size());
	for (auto const& scene : scenes) {
		auto& s = ret.scenes.emplace_back();
		auto const& root_nodes = scene["nodes"].array_view();
		s.root_nodes.reserve(root_nodes.size());
		for (auto const& node : root_nodes) { s.root_nodes.push_back(node.as<std::size_t>()); }
	}
	return ret;
}
} // namespace gltf
} // namespace facade
