#include "Biome.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "World/Noise.h"
#include "nlohmann/json.hpp"

namespace Struktur::World
{
Color Color::Lerp(const Color& from, const Color& to, float weight)
{
	return Color{from.red + (to.red - from.red) * weight, from.green + (to.green - from.green) * weight,
	             from.blue + (to.blue - from.blue) * weight};
}

static uint32_t ToByte(float value)
{
	return static_cast<uint32_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
}

uint32_t Color::ToAbgr(float alpha) const
{
	return (ToByte(alpha) << 24) | (ToByte(blue) << 16) | (ToByte(green) << 8) | ToByte(red);
}

const char* ToString(TerrainMaterial material)
{
	switch (material)
	{
		case TerrainMaterial::ROCK:
			return "rock";
		case TerrainMaterial::SNOW:
			return "snow";
		case TerrainMaterial::SAND:
			return "sand";
		case TerrainMaterial::WATER:
			return "water";
		default:
			return "grass";
	}
}

static TerrainMaterial MaterialFromString(const std::string& text)
{
	if (text == "rock")
	{
		return TerrainMaterial::ROCK;
	}
	if (text == "snow")
	{
		return TerrainMaterial::SNOW;
	}
	if (text == "sand")
	{
		return TerrainMaterial::SAND;
	}
	return TerrainMaterial::GRASS;
}

static PropSide SideFromString(const std::string& text)
{
	if (text == "left")
	{
		return PropSide::LEFT;
	}
	if (text == "right")
	{
		return PropSide::RIGHT;
	}
	if (text == "outer")
	{
		return PropSide::OUTER;
	}
	return PropSide::BOTH;
}

static void ReadNumber(const nlohmann::json& object, const char* key, double& value, double minimum, double maximum)
{
	if (object.is_object() && object.contains(key) && object[key].is_number())
	{
		const double read = object[key].get<double>();
		if (std::isfinite(read))
		{
			value = std::clamp(read, minimum, maximum);
		}
	}
}

static void ReadColor(const nlohmann::json& object, const char* key, Color& value)
{
	if (!object.is_object() || !object.contains(key))
	{
		return;
	}
	const nlohmann::json& entry = object[key];
	// [r, g, b] in 0-255 or "#rrggbb".
	if (entry.is_array() && entry.size() >= 3 && entry[0].is_number() && entry[1].is_number() && entry[2].is_number())
	{
		value.red   = std::clamp(entry[0].get<float>() / 255.0f, 0.0f, 1.0f);
		value.green = std::clamp(entry[1].get<float>() / 255.0f, 0.0f, 1.0f);
		value.blue  = std::clamp(entry[2].get<float>() / 255.0f, 0.0f, 1.0f);
	}
	else if (entry.is_string())
	{
		const std::string text = entry.get<std::string>();
		if (text.size() == 7 && text[0] == '#')
		{
			const unsigned long packed = std::strtoul(text.c_str() + 1, nullptr, 16);
			value.red                  = static_cast<float>((packed >> 16) & 0xFF) / 255.0f;
			value.green                = static_cast<float>((packed >> 8) & 0xFF) / 255.0f;
			value.blue                 = static_cast<float>(packed & 0xFF) / 255.0f;
		}
	}
}

static void ReadString(const nlohmann::json& object, const char* key, std::string& value)
{
	if (object.is_object() && object.contains(key) && object[key].is_string())
	{
		std::string read = object[key].get<std::string>();
		if (read.size() <= 128)
		{
			value = read;
		}
	}
}

static void ReadBool(const nlohmann::json& object, const char* key, bool& value)
{
	if (object.is_object() && object.contains(key) && object[key].is_boolean())
	{
		value = object[key].get<bool>();
	}
}

static constexpr size_t MAX_PALETTE_ENTRIES = 16;
static constexpr size_t MAX_PROP_RULES      = 32;

bool ParseBiome(const std::string& json, BiomeDefinition& out, std::string& error)
{
	const nlohmann::json root = nlohmann::json::parse(json, nullptr, false);
	if (root.is_discarded() || !root.is_object())
	{
		error = "not a JSON object";
		return false;
	}
	BiomeDefinition biome;
	ReadString(root, "id", biome.id);
	if (biome.id.empty())
	{
		error = "missing \"id\"";
		return false;
	}
	biome.name = biome.id;
	ReadString(root, "name", biome.name);
	ReadNumber(root, "weight", biome.weight, 0.0, 100.0);
	if (root.contains("lengthMeters") && root["lengthMeters"].is_array() && root["lengthMeters"].size() == 2 &&
	    root["lengthMeters"][0].is_number() && root["lengthMeters"][1].is_number())
	{
		biome.minLengthMeters = std::clamp(root["lengthMeters"][0].get<double>(), 800.0, 100000.0);
		biome.maxLengthMeters = std::clamp(root["lengthMeters"][1].get<double>(), biome.minLengthMeters, 100000.0);
	}

	const nlohmann::json sky = root.value("sky", nlohmann::json::object());
	ReadColor(sky, "zenith", biome.skyZenith);
	ReadColor(sky, "horizon", biome.skyHorizon);
	ReadColor(sky, "ground", biome.skyGround);
	ReadNumber(sky, "stars", biome.starIntensity, 0.0, 1.0);
	ReadNumber(sky, "moon", biome.moonIntensity, 0.0, 1.0);
	ReadNumber(sky, "clouds", biome.cloudAmount, 0.0, 1.0);

	const nlohmann::json sun = root.value("sun", nlohmann::json::object());
	ReadColor(sun, "color", biome.sunColor);
	ReadNumber(sun, "timeOfDay", biome.timeOfDay, 0.0, 24.0);
	ReadNumber(sun, "azimuth", biome.sunAzimuthDeg, -360.0, 360.0);
	ReadNumber(sun, "size", biome.sunSize, 0.2, 4.0);
	ReadColor(root, "ambient", biome.ambient);

	const nlohmann::json fog = root.value("fog", nlohmann::json::object());
	ReadColor(fog, "color", biome.fogColor);
	ReadNumber(fog, "density", biome.fogDensity, 0.0, 0.05);
	ReadNumber(fog, "start", biome.fogStartMeters, 0.0, 2000.0);

	const nlohmann::json mountains = root.value("mountains", nlohmann::json::object());
	ReadColor(mountains, "far", biome.mountainFar);
	ReadColor(mountains, "near", biome.mountainNear);
	ReadNumber(mountains, "height", biome.mountainHeight, 0.0, 0.4);

	const nlohmann::json terrain = root.value("terrain", nlohmann::json::object());
	ReadNumber(terrain, "amplitude", biome.terrainAmplitude, 0.0, 200.0);
	ReadNumber(terrain, "scale", biome.terrainScale, 20.0, 5000.0);
	ReadNumber(terrain, "corridor", biome.corridorMeters, 0.0, 30.0);
	ReadNumber(terrain, "blend", biome.blendMeters, 1.0, 400.0);
	ReadNumber(terrain, "leftOffset", biome.leftOffset, -200.0, 200.0);
	ReadNumber(terrain, "rightOffset", biome.rightOffset, -200.0, 200.0);
	if (terrain.is_object() && terrain.contains("palette") && terrain["palette"].is_array())
	{
		for (const nlohmann::json& item : terrain["palette"])
		{
			if (!item.is_object() || biome.palette.size() >= MAX_PALETTE_ENTRIES)
			{
				continue;
			}
			PaletteEntry entry;
			std::string material = "grass";
			ReadString(item, "material", material);
			entry.material = MaterialFromString(material);
			ReadColor(item, "color", entry.color);
			ReadNumber(item, "minHeight", entry.minHeight, -1e9, 1e9);
			ReadNumber(item, "maxHeight", entry.maxHeight, -1e9, 1e9);
			ReadNumber(item, "minSlope", entry.minSlope, 0.0, 1e9);
			ReadNumber(item, "maxSlope", entry.maxSlope, 0.0, 1e9);
			biome.palette.push_back(entry);
		}
	}
	if (biome.palette.empty())
	{
		PaletteEntry grass;
		grass.color = Color{0.35f, 0.55f, 0.25f};
		biome.palette.push_back(grass);
	}

	const nlohmann::json road = root.value("road", nlohmann::json::object());
	ReadColor(road, "asphalt", biome.roadAsphalt);
	ReadColor(road, "shoulder", biome.roadShoulder);
	ReadColor(road, "line", biome.roadLine);

	const nlohmann::json water = root.value("water", nlohmann::json::object());
	ReadBool(water, "enabled", biome.waterEnabled);
	ReadNumber(water, "level", biome.waterLevel, -100.0, -0.5);
	ReadColor(water, "color", biome.waterColor);
	std::string waterSide = "both";
	ReadString(water, "side", waterSide);
	biome.waterSide = SideFromString(waterSide);

	const nlohmann::json weather = root.value("weather", nlohmann::json::object());
	ReadString(weather, "type", biome.weather);
	if (biome.weather != "rain" && biome.weather != "snow")
	{
		biome.weather = "none";
	}
	ReadNumber(weather, "intensity", biome.weatherIntensity, 0.0, 1.0);

	const nlohmann::json grade = root.value("grade", nlohmann::json::object());
	ReadNumber(grade, "amplitude", biome.gradeAmplitudePercent, 0.0, 25.0);
	ReadNumber(grade, "wavelength", biome.gradeWavelengthMeters, 100.0, 20000.0);

	if (root.contains("props") && root["props"].is_array())
	{
		for (const nlohmann::json& item : root["props"])
		{
			if (!item.is_object() || biome.props.size() >= MAX_PROP_RULES)
			{
				continue;
			}
			PropRule rule;
			ReadString(item, "role", rule.role);
			if (rule.role.empty())
			{
				continue;
			}
			std::string placement = "scatter";
			ReadString(item, "placement", placement);
			rule.placement       = placement == "edge" ? PropPlacement::EDGE : PropPlacement::SCATTER;
			std::string side     = "both";
			ReadString(item, "side", side);
			rule.side = SideFromString(side);
			ReadNumber(item, "density", rule.density, 0.0, 200.0);
			ReadNumber(item, "spacing", rule.spacing, 2.0, 5000.0);
			if (item.contains("lateral") && item["lateral"].is_array() && item["lateral"].size() == 2 &&
			    item["lateral"][0].is_number() && item["lateral"][1].is_number())
			{
				rule.minLateral = std::clamp(item["lateral"][0].get<double>(), 0.0, 2000.0);
				rule.maxLateral = std::clamp(item["lateral"][1].get<double>(), rule.minLateral, 2000.0);
			}
			if (item.contains("height") && item["height"].is_array() && item["height"].size() == 2 &&
			    item["height"][0].is_number() && item["height"][1].is_number())
			{
				rule.minHeightMeters = std::clamp(item["height"][0].get<double>(), 0.05, 200.0);
				rule.maxHeightMeters = std::clamp(item["height"][1].get<double>(), rule.minHeightMeters, 200.0);
			}
			ReadNumber(item, "maxDistance", rule.maxDistance, 10.0, 5000.0);
			ReadNumber(item, "minCurvature", rule.minCurvature, 0.0, 1.0);
			ReadBool(item, "alignToRoad", rule.alignToRoad);
			biome.props.push_back(rule);
		}
	}
	out = std::move(biome);
	return true;
}

BiomeLook LookFor(const std::vector<BiomeDefinition>& biomes, const BiomeBlend& blend)
{
	BiomeLook look;
	if (biomes.empty())
	{
		return look;
	}
	const BiomeDefinition& first = biomes[static_cast<size_t>(std::clamp(blend.from, 0, static_cast<int>(biomes.size()) - 1))];
	const BiomeDefinition& second = biomes[static_cast<size_t>(std::clamp(blend.to, 0, static_cast<int>(biomes.size()) - 1))];
	const float weight       = blend.weight;
	const double mix         = weight;
	look.skyZenith           = Color::Lerp(first.skyZenith, second.skyZenith, weight);
	look.skyHorizon          = Color::Lerp(first.skyHorizon, second.skyHorizon, weight);
	look.skyGround           = Color::Lerp(first.skyGround, second.skyGround, weight);
	look.sunColor            = Color::Lerp(first.sunColor, second.sunColor, weight);
	look.ambient             = Color::Lerp(first.ambient, second.ambient, weight);
	look.fogColor            = Color::Lerp(first.fogColor, second.fogColor, weight);
	look.mountainFar         = Color::Lerp(first.mountainFar, second.mountainFar, weight);
	look.mountainNear        = Color::Lerp(first.mountainNear, second.mountainNear, weight);
	look.waterColor          = Color::Lerp(first.waterColor, second.waterColor, weight);
	look.timeOfDay           = first.timeOfDay + (second.timeOfDay - first.timeOfDay) * mix;
	look.sunAzimuthDeg       = first.sunAzimuthDeg + (second.sunAzimuthDeg - first.sunAzimuthDeg) * mix;
	look.sunSize             = first.sunSize + (second.sunSize - first.sunSize) * mix;
	look.starIntensity       = first.starIntensity + (second.starIntensity - first.starIntensity) * mix;
	look.moonIntensity       = first.moonIntensity + (second.moonIntensity - first.moonIntensity) * mix;
	look.fogDensity          = first.fogDensity + (second.fogDensity - first.fogDensity) * mix;
	look.fogStartMeters      = first.fogStartMeters + (second.fogStartMeters - first.fogStartMeters) * mix;
	look.mountainHeight      = first.mountainHeight + (second.mountainHeight - first.mountainHeight) * mix;
	look.cloudAmount         = first.cloudAmount + (second.cloudAmount - first.cloudAmount) * mix;
	const double rainA       = first.weather == "rain" ? first.weatherIntensity : 0.0;
	const double rainB       = second.weather == "rain" ? second.weatherIntensity : 0.0;
	const double snowA       = first.weather == "snow" ? first.weatherIntensity : 0.0;
	const double snowB       = second.weather == "snow" ? second.weatherIntensity : 0.0;
	look.rain                = rainA + (rainB - rainA) * mix;
	look.snow                = snowA + (snowB - snowA) * mix;
	return look;
}

BiomeSequence::BiomeSequence(const BiomeList& biomes, uint64_t seed, const std::string& forcedBiome,
                             double transitionMeters)
    : m_biomes(&biomes),
      m_seed(seed),
      m_forced(-1),
      m_transition(std::max(1.0, transitionMeters))
{
	const std::vector<BiomeDefinition>& list = *m_biomes;
	for (size_t index = 0; index < list.size(); ++index)
	{
		if (list[index].id == forcedBiome)
		{
			m_forced = static_cast<int>(index);
		}
	}
	m_regionStarts.push_back(0.0);
	m_regionBiomes.push_back(m_forced >= 0 ? m_forced : PickBiome(0, -1));
}

const std::vector<BiomeDefinition>& BiomeSequence::GetBiomes() const
{
	return *m_biomes;
}

const BiomeList& BiomeSequence::GetBiomeList() const
{
	return *m_biomes;
}

int BiomeSequence::PickBiome(int64_t region, int previous) const
{
	const std::vector<BiomeDefinition>& list = *m_biomes;
	if (list.empty())
	{
		return 0;
	}
	double total = 0.0;
	for (size_t index = 0; index < list.size(); ++index)
	{
		if (static_cast<int>(index) != previous || list.size() == 1)
		{
			total += list[index].weight;
		}
	}
	if (total <= 0.0)
	{
		return previous < 0 ? 0 : previous;
	}
	double pick = HashUnit(m_seed ^ 0xB10Eu, region) * total;
	for (size_t index = 0; index < list.size(); ++index)
	{
		if (static_cast<int>(index) == previous && list.size() > 1)
		{
			continue;
		}
		pick -= list[index].weight;
		if (pick < 0.0)
		{
			return static_cast<int>(index);
		}
	}
	return static_cast<int>(list.size()) - 1;
}

void BiomeSequence::ExtendTo(double distance)
{
	const std::vector<BiomeDefinition>& list = *m_biomes;
	while (m_regionStarts.back() <= distance + m_transition)
	{
		const int64_t region = static_cast<int64_t>(m_regionStarts.size()) - 1;
		const int biome      = m_regionBiomes.back();
		double length        = 4000.0;
		if (!list.empty())
		{
			const BiomeDefinition& definition = list[static_cast<size_t>(biome)];
			length = definition.minLengthMeters +
			         (definition.maxLengthMeters - definition.minLengthMeters) * HashUnit(m_seed ^ 0x1E6u, region);
		}
		m_regionStarts.push_back(m_regionStarts.back() + length);
		m_regionBiomes.push_back(m_forced >= 0 ? m_forced : PickBiome(region + 1, biome));
	}
}

BiomeBlend BiomeSequence::BlendAt(double distance)
{
	BiomeBlend blend;
	if (m_biomes->empty())
	{
		return blend;
	}
	distance = std::max(0.0, distance);
	ExtendTo(distance);
	// Region containing distance.
	const auto found = std::upper_bound(m_regionStarts.begin(), m_regionStarts.end(), distance);
	const size_t region = static_cast<size_t>(std::max<ptrdiff_t>(0, (found - m_regionStarts.begin()) - 1));
	blend.from          = m_regionBiomes[region];
	blend.to            = m_regionBiomes[region + 1];
	const double end    = m_regionStarts[region + 1];
	const double into   = distance - (end - m_transition);
	if (into > 0.0)
	{
		const double fraction = std::clamp(into / m_transition, 0.0, 1.0);
		blend.weight          = static_cast<float>(fraction * fraction * (3.0 - 2.0 * fraction));
	}
	if (blend.weight <= 0.0f)
	{
		blend.to = blend.from;
	}
	return blend;
}

double BiomeSequence::RegionStart(double distance)
{
	ExtendTo(distance);
	const auto found = std::upper_bound(m_regionStarts.begin(), m_regionStarts.end(), distance);
	return m_regionStarts[static_cast<size_t>(std::max<ptrdiff_t>(0, (found - m_regionStarts.begin()) - 1))];
}
}  // namespace Struktur::World
