#pragma once
#include <facade/vk/shader.hpp>

namespace facade::shaders {
Shader unlit();
Shader lit();
Shader skybox();
Shader skinned();
} // namespace facade::shaders
