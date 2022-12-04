#pragma once
#include <facade/vk/shader.hpp>

namespace facade {
namespace vert {
Shader default_();
Shader skinned();
} // namespace vert

namespace frag {
Shader unlit();
Shader lit();
Shader skybox();
} // namespace frag
} // namespace facade
