#include "SpriteDescriptor.h"

#include <algorithm>

#include "nlohmann/json.hpp"

namespace Struktur::World
{
static constexpr size_t MAX_LAYERS = 16;

static int ReadInt(const nlohmann::json& object, const char* key, int fallback, int minimum, int maximum)
{
	if (object.is_object() && object.contains(key) && object[key].is_number_integer())
	{
		return std::clamp(object[key].get<int>(), minimum, maximum);
	}
	return fallback;
}

static std::string ReadText(const nlohmann::json& object, const char* key, const std::string& fallback)
{
	if (object.is_object() && object.contains(key) && object[key].is_string())
	{
		std::string text = object[key].get<std::string>();
		if (text.size() <= 128)
		{
			return text;
		}
	}
	return fallback;
}

static SpriteLayerMotion MotionFromString(const std::string& text)
{
	if (text == "crank")
	{
		return SpriteLayerMotion::CRANK;
	}
	if (text == "rotate")
	{
		return SpriteLayerMotion::WHEEL_ROTATE;
	}
	if (text == "scroll")
	{
		return SpriteLayerMotion::WHEEL_SCROLL;
	}
	return SpriteLayerMotion::STATIC;
}

static void ReadRows(const nlohmann::json& object, SpriteLayer& layer)
{
	if (!object.is_object() || !object.contains("rows") || !object["rows"].is_object())
	{
		return;
	}
	const nlohmann::json& rows = object["rows"];
	layer.pedalRow             = ReadInt(rows, "pedal", layer.pedalRow, 0, 63);
	layer.outOfSaddleRow       = ReadInt(rows, "outOfSaddle", layer.outOfSaddleRow, -1, 63);
	layer.stillRow             = ReadInt(rows, "still", layer.stillRow, -1, 63);
}

bool ParseSpriteDescriptor(const std::string& json, SpriteDescriptor& out, std::string& error)
{
	const nlohmann::json root = nlohmann::json::parse(json, nullptr, false);
	if (root.is_discarded() || !root.is_object())
	{
		error = "not a JSON object";
		return false;
	}
	SpriteDescriptor descriptor;
	descriptor.kind = ReadText(root, "kind", "layered");
	if (descriptor.kind != "layered" && descriptor.kind != "sheet")
	{
		error = "kind must be \"layered\" or \"sheet\"";
		return false;
	}
	if (root.contains("frameSize") && root["frameSize"].is_array() && root["frameSize"].size() == 2 &&
	    root["frameSize"][0].is_number_integer() && root["frameSize"][1].is_number_integer())
	{
		descriptor.frameWidth  = std::clamp(root["frameSize"][0].get<int>(), 4, 2048);
		descriptor.frameHeight = std::clamp(root["frameSize"][1].get<int>(), 4, 2048);
	}
	descriptor.framesPerRevolution = ReadInt(root, "framesPerRevolution", descriptor.framesPerRevolution, 1, 128);
	if (root.contains("anchor") && root["anchor"].is_array() && root["anchor"].size() == 2 &&
	    root["anchor"][0].is_number_integer() && root["anchor"][1].is_number_integer())
	{
		descriptor.anchorX = std::clamp(root["anchor"][0].get<int>(), 0, descriptor.frameWidth);
		descriptor.anchorY = std::clamp(root["anchor"][1].get<int>(), 0, descriptor.frameHeight);
	}
	if (root.contains("screenHeight") && root["screenHeight"].is_number())
	{
		descriptor.screenHeight = std::clamp(root["screenHeight"].get<double>(), 0.05, 0.9);
	}
	if (root.contains("heightMeters") && root["heightMeters"].is_number())
	{
		descriptor.heightMeters = std::clamp(root["heightMeters"].get<double>(), 0.5, 4.0);
	}
	if (root.contains("lean") && root["lean"].is_boolean())
	{
		descriptor.lean = root["lean"].get<bool>();
	}
	descriptor.leanLeftRow  = ReadInt(root, "leanLeftRow", -1, -1, 63);
	descriptor.leanRightRow = ReadInt(root, "leanRightRow", -1, -1, 63);

	if (root.contains("layers") && root["layers"].is_array())
	{
		for (const nlohmann::json& item : root["layers"])
		{
			if (!item.is_object() || descriptor.layers.size() >= MAX_LAYERS)
			{
				continue;
			}
			SpriteLayer layer;
			layer.name   = ReadText(item, "name", "layer");
			layer.role   = ReadText(item, "role", "");
			layer.motion = MotionFromString(ReadText(item, "motion", "static"));
			if (layer.role.empty())
			{
				continue;
			}
			if (item.contains("rect") && item["rect"].is_array() && item["rect"].size() == 4)
			{
				const nlohmann::json& rect = item["rect"];
				if (rect[0].is_number_integer() && rect[1].is_number_integer() && rect[2].is_number_integer() &&
				    rect[3].is_number_integer())
				{
					layer.rectX      = std::clamp(rect[0].get<int>(), -descriptor.frameWidth * 2, descriptor.frameWidth * 2);
					layer.rectY      = std::clamp(rect[1].get<int>(), -descriptor.frameHeight * 2, descriptor.frameHeight * 2);
					layer.rectWidth  = std::clamp(rect[2].get<int>(), 0, descriptor.frameWidth * 4);
					layer.rectHeight = std::clamp(rect[3].get<int>(), 0, descriptor.frameHeight * 4);
				}
			}
			layer.spokes             = ReadInt(item, "spokes", 0, 0, 64);
			layer.framesPerRevolution = ReadInt(item, "framesPerRevolution", 0, 0, 128);
			layer.coastColumn        = ReadInt(item, "coastColumn", 0, 0, 127);
			layer.idleColumn         = ReadInt(item, "idleColumn", 0, 0, 127);
			layer.outOfSaddleOffsetY = ReadInt(item, "outOfSaddleOffsetY", 0, -256, 256);
			ReadRows(item, layer);
			descriptor.layers.push_back(layer);
		}
	}
	if (descriptor.kind == "sheet" && descriptor.layers.empty())
	{
		error = "a sheet descriptor needs one layer naming the sheet's role";
		return false;
	}
	if (descriptor.layers.empty())
	{
		error = "no layers";
		return false;
	}
	out = std::move(descriptor);
	return true;
}

SpriteDescriptor DefaultSpriteDescriptor()
{
	SpriteDescriptor descriptor;
	SpriteLayer wheel;
	wheel.name       = "wheels";
	wheel.role       = "rider_wheel";
	wheel.motion     = SpriteLayerMotion::WHEEL_SCROLL;
	wheel.rectX      = 29;
	wheel.rectY      = 58;
	wheel.rectWidth  = 6;
	wheel.rectHeight = 38;
	SpriteLayer frame;
	frame.name   = "frame";
	frame.role   = "rider_frame";
	SpriteLayer legs;
	legs.name           = "legs";
	legs.role           = "rider_legs";
	legs.motion         = SpriteLayerMotion::CRANK;
	legs.pedalRow       = 0;
	legs.outOfSaddleRow = 1;
	legs.stillRow       = 2;
	legs.coastColumn    = 0;
	legs.idleColumn     = 1;
	SpriteLayer torso;
	torso.name               = "torso";
	torso.role               = "rider_torso";
	torso.outOfSaddleOffsetY = -7;
	descriptor.layers = {wheel, frame, legs, torso};
	return descriptor;
}
}  // namespace Struktur::World
