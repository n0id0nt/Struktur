#pragma once

#include <string>
#include <vector>

#include "World/World.h"

// The engine side of WorldAssets: reads assets/World through the engine file system. The World library itself never
// touches files.
namespace Struktur::World
{
// Biomes, art manifest and rider sprite. A problem is logged and that part falls back to a placeholder or is left out.
WorldAssets LoadWorldAssets();
// Every manifest role whose real file is missing, as log lines / a Markdown table (--list-missing-assets).
std::vector<std::string> GetMissingAssetLines(const AssetManifest& manifest);
std::string GetMissingAssetsMarkdown(const AssetManifest& manifest);
}  // namespace Struktur::World
