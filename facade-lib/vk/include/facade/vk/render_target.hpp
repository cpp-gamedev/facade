#pragma once
#include <facade/util/flex_array.hpp>
#include <facade/vk/vma.hpp>

namespace facade {
using RenderAttachments = FlexArray<vk::ImageView, 3>;

struct RenderTarget {
	ImageView colour{};
	ImageView depth{};
	ImageView resolve{};
	vk::Extent2D extent{};

	RenderAttachments attachments() const {
		auto ret = RenderAttachments{};
		if (colour.view) { ret.insert(colour.view); }
		if (depth.view) { ret.insert(depth.view); }
		if (resolve.view) { ret.insert(resolve.view); }
		return ret;
	}

	void to_draw(vk::CommandBuffer cb);
	void to_present(vk::CommandBuffer cb);
};

struct Framebuffer {
	RenderAttachments attachments{};
	vk::Framebuffer framebuffer{};
	vk::CommandBuffer primary{};
	std::span<vk::CommandBuffer const> secondary{};
};
} // namespace facade
