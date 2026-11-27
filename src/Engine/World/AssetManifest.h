#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Struktur::World
{
// One art role the world needs (assets/World/manifest.json). `file` is where a real asset is expected; when it is
// missing a placeholder is generated in code from `placeholderShape`, so the game always runs. Replacing a
// placeholder = dropping a file at `file` (or pointing `file` somewhere else).
struct AssetRole
{
	std::string role;
	std::string category;  // rider, props, sky, terrain, road, particles - for grouping in docs and logs
	std::string file;      // path in the engine file system (relative to assets/)
	int width             = 64;  // suggested size of ONE frame
	int height            = 64;
	std::string format    = "PNG, RGBA (straight alpha)";
	int frames            = 1;   // frames / variants laid out left to right
	int rows              = 1;   // rows of frames (e.g. rider legs: pedalling, out of saddle, still poses)
	int framesPerRevolution = 0;  // rider crank layers: frames per crank revolution
	// Optional: cut the frames out of a larger sheet (a tileset, a character atlas). Each region is one frame
	// (x, y, width, height in the file's pixels), laid out left to right in this order; frames = regions.size().
	std::vector<std::array<int, 4>> regions;
	int anchorX           = -1;  // pixel in a frame that sits on the ground / attaches; -1 = bottom centre
	int anchorY           = -1;
	bool tileable         = false;
	std::string notes;
	std::string placeholderShape;  // see Placeholder.h
	uint32_t placeholderColor = 0xFFFFFFFFu;  // 0xRRGGBBAA
};

struct AssetManifest
{
	int version = 1;
	std::vector<AssetRole> roles;

	const AssetRole* Find(const std::string& role) const;
};

// Returns false (and `error`) when the text isn't a manifest; bad entries are skipped and listed in `warnings`.
bool ParseManifest(const std::string& json, AssetManifest& out, std::string& error, std::vector<std::string>& warnings);

using FileExistsFunction = bool (*)(const std::string& path);

struct ResolvedAsset
{
	const AssetRole* role = nullptr;
	bool found            = false;  // the real file exists
	std::string path;               // the real file when found
	std::string placeholderName;    // "placeholder_<role>"
};

std::string PlaceholderName(const std::string& role);
std::vector<ResolvedAsset> ResolveAssets(const AssetManifest& manifest, FileExistsFunction exists);
// Every missing role as a Markdown table (role, expected file, size and format, frames, anchor, placeholder, notes).
std::string FormatMissingAssetsMarkdown(const std::vector<ResolvedAsset>& resolved);
// The same list as plain log lines.
std::vector<std::string> FormatMissingAssetsLines(const std::vector<ResolvedAsset>& resolved);
}  // namespace Struktur::World
