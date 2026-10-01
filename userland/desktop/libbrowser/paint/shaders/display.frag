// zedBSD browser: colors one pixel of a display item (paint/vulkan.c).
//
// The coverage rules are the CPU reference renderer's (paint/software.c): a
// rectangle covers a pixel by the area of the pixel square it overlaps, and a
// glyph's coverage is its bitmap's texel at the pixel (the glyph's rectangle is
// on whole pixels).  An image covers a pixel as a rectangle does, with the
// color of its texel under the pixel's centre (the same steps as the CPU
// renderer's, in single precision).  The color goes out straight, its alpha
// times the coverage, and the blend state mixes it over the target.
// Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#version 450

// The atlas: each glyph's coverage in every channel, and the images' pixels.
layout(set = 0, binding = 0) uniform sampler2D glyphs;

layout(location = 0) flat in vec4 item_rect;
layout(location = 1) flat in vec4 item_color;
layout(location = 2) flat in vec4 item_atlas;
layout(location = 3) flat in vec4 item_source;
layout(location = 4) flat in vec4 item_texels;

layout(location = 0) out vec4 color;

void main()
{
	vec2 pixel = floor(gl_FragCoord.xy);
	float coverage;

	if (item_atlas.z > 1.5) {
		// An image: the rectangle's overlap, and the texel under the pixel's centre.
		vec2 low = max(item_rect.xy, pixel);
		vec2 high = min(item_rect.zw, pixel + 1.0);
		vec2 overlap = clamp(high - low, 0.0, 1.0);
		vec2 place = floor((pixel + 0.5 - item_source.xy) * item_source.zw);
		place = clamp(place, vec2(0.0), item_texels.xy - 1.0);
		vec4 texel = texelFetch(glyphs, ivec2(item_atlas.xy + place), 0);
		color = vec4(texel.rgb, texel.a * overlap.x * overlap.y);
		return;
	}

	if (item_atlas.z < 0.5) {
		// A rectangle: the overlap of [pixel, pixel + 1) with it, on each axis.
		vec2 low = max(item_rect.xy, pixel);
		vec2 high = min(item_rect.zw, pixel + 1.0);
		vec2 overlap = clamp(high - low, 0.0, 1.0);
		coverage = overlap.x * overlap.y;
	} else {
		// A glyph: its texel at this pixel.
		ivec2 texel = ivec2(item_atlas.xy + pixel - item_rect.xy);
		coverage = texelFetch(glyphs, texel, 0).a;
	}

	color = vec4(item_color.rgb, item_color.a * coverage);
}
