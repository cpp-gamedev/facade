#pragma once
#include <facade/util/enum_array.hpp>
#include <string_view>

namespace facade {
///
/// \brief Loading stage of a GLTF asset into a Scene.
///
enum class LoadStage : std::uint8_t {
	eNone,
	eParsingJson,
	eLoadingImages,
	eUploadingTextures,
	eUploadingMeshes,
	eBuildingScenes,
	eCOUNT_,
};

///
/// \brief String map for LoadStage.
///
constexpr auto load_stage_str = EnumArray<LoadStage, std::string_view>{
	"None", "Parsing JSON", "Loading Images", "Uploading Textures", "Uploading Meshes", "Building Scenes",
};
static_assert(std::size(load_stage_str.t) == static_cast<std::size_t>(LoadStage::eCOUNT_));

struct LoadStatus {
	LoadStage stage{};
	std::size_t total{};
	std::size_t done{};

	///
	/// \brief Obtain the overall loading progress.
	///
	constexpr float load_progress() const {
		if (total == 0) { return 0.0f; }
		return static_cast<float>(done) / static_cast<float>(total);
	}
};
} // namespace facade
