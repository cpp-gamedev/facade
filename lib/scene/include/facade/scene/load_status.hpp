#pragma once
#include <facade/util/enum_array.hpp>
#include <string_view>

namespace facade {
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

constexpr float load_progress(LoadStatus const stage) { return static_cast<float>(stage) / static_cast<float>(LoadStatus::eCOUNT_); }
} // namespace facade
