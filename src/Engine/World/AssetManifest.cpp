#include "AssetManifest.h"

#include <algorithm>
#include <cstdlib>

#include "nlohmann/json.hpp"

namespace Struktur::World
{
static constexpr size_t MAX_ROLES       = 256;
static constexpr int MAX_IMAGE_SIZE     = 4096;
static constexpr int MAX_FRAMES         = 256;

const AssetRole* AssetManifest::Find(const std::string& role) const
{
	for (const AssetRole& entry : roles)
	{
		if (entry.role == role)
		{
			return &entry;
		}
	}
	return nullptr;
}

static bool SafePath(const std::string& path)
{
	// Engine paths only: relative, no parent references, no drive letters.
	return !path.empty() && path.size() < 260 && path.find("..") == std::string::npos && path[0] != '/' &&
	       path[0] != '\\' && path.find(':') == std::string::npos;
}

static uint32_t ParseColor(const std::string& text, uint32_t fallback)
{
	if ((text.size() == 7 || text.size() == 9) && text[0] == '#')
	{
		const unsigned long packed = std::strtoul(text.c_str() + 1, nullptr, 16);
		return text.size() == 7 ? static_cast<uint32_t>((packed << 8) | 0xFFu) : static_cast<uint32_t>(packed);
	}
	return fallback;
}

static int ReadInt(const nlohmann::json& object, const char* key, int fallback, int minimum, int maximum)
{
	if (object.contains(key) && object[key].is_number_integer())
	{
		return std::clamp(object[key].get<int>(), minimum, maximum);
	}
	return fallback;
}

static std::string ReadText(const nlohmann::json& object, const char* key, const std::string& fallback)
{
	if (object.contains(key) && object[key].is_string())
	{
		std::string text = object[key].get<std::string>();
		if (text.size() <= 1024)
		{
			return text;
		}
	}
	return fallback;
}

bool ParseManifest(const std::string& json, AssetManifest& out, std::string& error, std::vector<std::string>& warnings)
{
	const nlohmann::json root = nlohmann::json::parse(json, nullptr, false);
	if (root.is_discarded() || !root.is_object() || !root.contains("roles") || !root["roles"].is_array())
	{
		error = "manifest must be a JSON object with a \"roles\" array";
		return false;
	}
	AssetManifest manifest;
	manifest.version = ReadInt(root, "version", 1, 1, 100);
	for (const nlohmann::json& item : root["roles"])
	{
		if (manifest.roles.size() >= MAX_ROLES)
		{
			warnings.push_back("too many roles - the rest are ignored");
			break;
		}
		if (!item.is_object())
		{
			continue;
		}
		AssetRole role;
		role.role = ReadText(item, "role", "");
		role.file = ReadText(item, "file", "");
		if (role.role.empty() || manifest.Find(role.role) != nullptr)
		{
			warnings.push_back("skipped a role with no name or a duplicate name ('" + role.role + "')");
			continue;
		}
		if (!SafePath(role.file))
		{
			warnings.push_back("role '" + role.role + "' has an unsafe or empty file path - placeholder only");
			role.file.clear();
		}
		role.category = ReadText(item, "category", "misc");
		if (item.contains("size") && item["size"].is_array() && item["size"].size() == 2 &&
		    item["size"][0].is_number_integer() && item["size"][1].is_number_integer())
		{
			role.width  = std::clamp(item["size"][0].get<int>(), 1, MAX_IMAGE_SIZE);
			role.height = std::clamp(item["size"][1].get<int>(), 1, MAX_IMAGE_SIZE);
		}
		role.format              = ReadText(item, "format", role.format);
		role.frames              = ReadInt(item, "frames", 1, 1, MAX_FRAMES);
		role.rows                = ReadInt(item, "rows", 1, 1, 64);
		role.framesPerRevolution = ReadInt(item, "framesPerRevolution", 0, 0, MAX_FRAMES);
		if (item.contains("anchor") && item["anchor"].is_array() && item["anchor"].size() == 2 &&
		    item["anchor"][0].is_number_integer() && item["anchor"][1].is_number_integer())
		{
			role.anchorX = std::clamp(item["anchor"][0].get<int>(), 0, role.width);
			role.anchorY = std::clamp(item["anchor"][1].get<int>(), 0, role.height);
		}
		role.tileable = item.contains("tileable") && item["tileable"].is_boolean() && item["tileable"].get<bool>();
		if (item.contains("regions") && item["regions"].is_array())
		{
			for (const nlohmann::json& region : item["regions"])
			{
				if (!region.is_array() || region.size() != 4 || !region[0].is_number_integer() ||
				    !region[1].is_number_integer() || !region[2].is_number_integer() || !region[3].is_number_integer())
				{
					warnings.push_back("role '" + role.role + "' has a malformed region - ignored");
					continue;
				}
				const std::array<int, 4> rect = {std::clamp(region[0].get<int>(), 0, MAX_IMAGE_SIZE),
				                                 std::clamp(region[1].get<int>(), 0, MAX_IMAGE_SIZE),
				                                 std::clamp(region[2].get<int>(), 1, MAX_IMAGE_SIZE),
				                                 std::clamp(region[3].get<int>(), 1, MAX_IMAGE_SIZE)};
				if (role.regions.size() < static_cast<size_t>(MAX_FRAMES))
				{
					role.regions.push_back(rect);
				}
			}
			if (!role.regions.empty())
			{
				role.frames = static_cast<int>(role.regions.size());
				role.rows   = 1;
			}
		}
		role.notes    = ReadText(item, "notes", "");
		if (item.contains("placeholder") && item["placeholder"].is_object())
		{
			const nlohmann::json& placeholder = item["placeholder"];
			role.placeholderShape             = ReadText(placeholder, "shape", "");
			role.placeholderColor = ParseColor(ReadText(placeholder, "color", ""), role.placeholderColor);
		}
		if (role.placeholderShape.empty())
		{
			role.placeholderShape = "box";
		}
		manifest.roles.push_back(role);
	}
	out = std::move(manifest);
	return true;
}

std::string PlaceholderName(const std::string& role)
{
	return "placeholder_" + role;
}

std::vector<ResolvedAsset> ResolveAssets(const AssetManifest& manifest, FileExistsFunction exists)
{
	std::vector<ResolvedAsset> resolved;
	resolved.reserve(manifest.roles.size());
	for (const AssetRole& role : manifest.roles)
	{
		ResolvedAsset asset;
		asset.role            = &role;
		asset.placeholderName = PlaceholderName(role.role);
		if (!role.file.empty() && exists != nullptr && exists(role.file))
		{
			asset.found = true;
			asset.path  = role.file;
		}
		resolved.push_back(asset);
	}
	return resolved;
}

static std::string FramesText(const AssetRole& role)
{
	std::string text = std::to_string(role.frames);
	if (role.rows > 1)
	{
		text += " x " + std::to_string(role.rows) + " rows";
	}
	if (role.framesPerRevolution > 0)
	{
		text += " (" + std::to_string(role.framesPerRevolution) + " per crank revolution)";
	}
	return text;
}

static std::string AnchorText(const AssetRole& role)
{
	if (role.anchorX < 0)
	{
		return "bottom centre";
	}
	return std::to_string(role.anchorX) + ", " + std::to_string(role.anchorY);
}

static std::string SizeText(const AssetRole& role)
{
	std::string text = std::to_string(role.width) + " x " + std::to_string(role.height);
	if (role.frames > 1 || role.rows > 1)
	{
		text += " per frame (sheet " + std::to_string(role.width * role.frames) + " x " +
		        std::to_string(role.height * role.rows) + ")";
	}
	if (role.tileable)
	{
		text += ", tileable";
	}
	return text;
}

static std::string EscapePipes(const std::string& text)
{
	std::string escaped;
	for (char character : text)
	{
		if (character == '|')
		{
			escaped += "\\|";
		}
		else
		{
			escaped += character;
		}
	}
	return escaped;
}

std::string FormatMissingAssetsMarkdown(const std::vector<ResolvedAsset>& resolved)
{
	std::string text =
	    "| Role | Category | Expected file | Size and format | Frames | Anchor | Placeholder used | Notes |\n"
	    "|---|---|---|---|---|---|---|---|\n";
	for (const ResolvedAsset& asset : resolved)
	{
		if (asset.found)
		{
			continue;
		}
		const AssetRole& role = *asset.role;
		text += "| `" + role.role + "` | " + role.category + " | `assets/" + role.file + "` | " + SizeText(role) + "; " +
		        EscapePipes(role.format) + " | " + FramesText(role) + " | " + AnchorText(role) + " | `" +
		        asset.placeholderName + "` (" + role.placeholderShape + ") | " + EscapePipes(role.notes) + " |\n";
	}
	return text;
}

std::vector<std::string> FormatMissingAssetsLines(const std::vector<ResolvedAsset>& resolved)
{
	std::vector<std::string> lines;
	for (const ResolvedAsset& asset : resolved)
	{
		if (asset.found)
		{
			continue;
		}
		const AssetRole& role = *asset.role;
		lines.push_back(role.role + ": expected assets/" + role.file + " (" + SizeText(role) + ", " + role.format +
		                ", frames " + FramesText(role) + ", anchor " + AnchorText(role) + ") - using " +
		                asset.placeholderName);
	}
	return lines;
}
}  // namespace Struktur::World
