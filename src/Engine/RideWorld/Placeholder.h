#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Engine/RideWorld/AssetManifest.h"

namespace Struktur::RideWorld
{
// An RGBA8 image (straight alpha, rows top to bottom).
struct PlaceholderImage
{
	int width  = 0;
	int height = 0;
	std::vector<uint8_t> pixels;
};

// Draws a placeholder for a role in code: simple, clearly-not-final shapes sized like the real asset should be
// (role.width x role.height per frame, frames x rows), so swapping in real art changes nothing else. Shapes:
//   tree_pine, tree_broadleaf, tree_palm, cactus, bush, rock, grass, sign, guardrail, lamp, building, banner, cloud,
//   sun, moon, stars, mountains, noise (tileable texture in placeholderColor), water, markings, rain, snow, shadow,
//   rider_frame, rider_wheel, rider_legs, rider_torso, box.
// Unknown shapes draw a magenta/black checker box.
PlaceholderImage GeneratePlaceholder(const AssetRole& role, uint64_t seed);

// Shapes GeneratePlaceholder knows (for the manifest test).
const std::vector<std::string>& KnownPlaceholderShapes();
}  // namespace Struktur::RideWorld
