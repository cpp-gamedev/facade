#pragma once
#include <facade/util/enum_array.hpp>
#include <string_view>

namespace facade {
///
/// \brief Loading status of a GLTF asset into a Scene.
///
enum class LoadStatus : std::uint8_t {
	eNone,
	eStartingThread,
	eParsingBuffers,
	eParsingBufferViews,
	eParsingAccessors,
	eParsingCameras,
	eParsingSamplers,
	eLoadingImages,
	eParsingTextures,
	eParsingMeshes,
	eParsingMaterials,
	eBuildingGeometry,
	eBuildingNodes,
	eBuildingScenes,
	eUploadingResources,
	eCOUNT_,
};

///
/// \brief String map for LoadStatus.
///
constexpr auto load_status_str = EnumArray<LoadStatus, std::string_view>{
	"None",
	"Starting Thread",
	"Parsing Buffers",
	"Parsing BufferViews",
	"Parsing Accessors",
	"Parsing Cameras",
	"Parsing Samplers",
	"Loading Images",
	"Parsing Textures",
	"Parsing Meshes",
	"Parsing Materials",
	"Building Geometry",
	"Building Nodes",
	"Building Scenes",
	"Uploading Resources",
};

static_assert(std::size(load_status_str.t) == static_cast<std::size_t>(LoadStatus::eCOUNT_));

///
/// \brief Obtain the overall loading progress.
///
constexpr float load_progress(LoadStatus const status) { return static_cast<float>(status) / static_cast<float>(LoadStatus::eCOUNT_); }
} // namespace facade
