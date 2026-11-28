#include "WorldAssetLoader.h"

#include <algorithm>
#include <cctype>

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"

namespace Struktur::World
{
static constexpr const char* MANIFEST_FILE        = "World/manifest.json";
static constexpr const char* BIOMES_FOLDER        = "World/Biomes";
static constexpr const char* RIDER_FILE           = "World/Rider/rider.json";
static constexpr size_t MAX_BIOME_FILE_BYTES      = 256 * 1024;

static bool IsJsonFile(std::string name)
{
	for (char& character : name)
	{
		character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
	}
	return name.ends_with(".json");
}

static bool EngineFileExists(const std::string& path)
{
	return FileSystem::Exists(path);
}

// Every .json in World/Biomes, sorted by file name (adding a biome is a data-only change).
static BiomeList LoadBiomes()
{
	BiomeList biomes;
	std::vector<std::string> files = FileSystem::ListDirectory(BIOMES_FOLDER);
	std::sort(files.begin(), files.end());
	for (const std::string& file : files)
	{
		if (!IsJsonFile(file))
		{
			continue;
		}
		const std::string path          = std::string(BIOMES_FOLDER) + "/" + file;
		const FileResult<uint64_t> size = FileSystem::GetFileSize(path);
		if (!size || size.value > MAX_BIOME_FILE_BYTES)
		{
			DEBUG_WARNING("World: skipping biome %s (unreadable or too large)", path.c_str());
			continue;
		}
		const auto text = FileSystem::ReadString(path);
		BiomeDefinition biome;
		std::string error;
		if (text && ParseBiome(text.value, biome, error))
		{
			biomes.push_back(std::move(biome));
		}
		else
		{
			DEBUG_WARNING("World: biome %s ignored: %s", path.c_str(), error.c_str());
		}
	}
	if (biomes.empty())
	{
		DEBUG_WARNING("World: no biomes in %s - the world is disabled", BIOMES_FOLDER);
	}
	return biomes;
}

static SpriteDescriptor LoadSpriteDescriptor()
{
	if (!FileSystem::Exists(RIDER_FILE))
	{
		return DefaultSpriteDescriptor();
	}
	const auto text = FileSystem::ReadString(RIDER_FILE);
	std::string error;
	SpriteDescriptor descriptor;
	if (text && ParseSpriteDescriptor(text.value, descriptor, error))
	{
		return descriptor;
	}
	DEBUG_WARNING("World: %s ignored (%s) - using the built-in rider", RIDER_FILE, error.c_str());
	return DefaultSpriteDescriptor();
}

static AssetManifest LoadManifest()
{
	AssetManifest manifest;
	if (!FileSystem::Exists(MANIFEST_FILE))
	{
		return manifest;
	}
	const auto text = FileSystem::ReadString(MANIFEST_FILE);
	std::string error;
	std::vector<std::string> warnings;
	if (!text || !ParseManifest(text.value, manifest, error, warnings))
	{
		DEBUG_WARNING("World: %s unreadable (%s) - every role is a placeholder", MANIFEST_FILE, error.c_str());
	}
	for (const std::string& warning : warnings)
	{
		DEBUG_WARNING("World manifest: %s", warning.c_str());
	}
	return manifest;
}

WorldAssets LoadWorldAssets()
{
	WorldAssets assets;
	assets.biomes           = LoadBiomes();
	assets.spriteDescriptor = LoadSpriteDescriptor();
	assets.manifest         = LoadManifest();
	// Logged once at startup so missing art is obvious (also: --list-missing-assets).
	const std::vector<std::string> missing = GetMissingAssetLines(assets.manifest);
	if (!missing.empty())
	{
		DEBUG_INFO("World: %d of %d art roles use placeholders:", (int)missing.size(), (int)assets.manifest.roles.size());
		for (const std::string& line : missing)
		{
			DEBUG_INFO("  missing %s", line.c_str());
		}
	}
	return assets;
}

std::vector<std::string> GetMissingAssetLines(const AssetManifest& manifest)
{
	return FormatMissingAssetsLines(ResolveAssets(manifest, EngineFileExists));
}

std::string GetMissingAssetsMarkdown(const AssetManifest& manifest)
{
	return FormatMissingAssetsMarkdown(ResolveAssets(manifest, EngineFileExists));
}
}  // namespace Struktur::World
