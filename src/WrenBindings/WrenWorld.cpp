// "world" module - the Wren face of the procedural world (World::World + its renderer). Scripts load and unload it
// explicitly, e.g. when the ride screen is entered and left.
#include <algorithm>
#include <string>

#include <nlohmann/json.hpp>

#include "Debug/Assertions.h"
#include "Engine/Callback/CallbackHelperFunctions.h"
#include "Engine/Callback/Variant.h"
#include "Engine/Core/CommandLine.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "wren.hpp"

namespace World = Struktur::World;

static Struktur::GameContext& ContextOf(WrenVM* vm)
{
	return *static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
}

// World settings arrive as the JSON text of a "world" section (Ride.worldSettings() makes one).
static bool SettingsFromSlot(WrenVM* vm, int slot, World::WorldSettings& settings)
{
	if (wrenGetSlotType(vm, slot) != WREN_TYPE_STRING)
	{
		return false;
	}
	const nlohmann::json section = nlohmann::json::parse(wrenGetSlotString(vm, slot), nullptr, false);
	if (!section.is_object())
	{
		return false;
	}
	std::string warning;
	settings = World::WorldSettings::FromJson(section, warning);
	if (!warning.empty())
	{
		DEBUG_WARNING("%s", warning.c_str());
	}
	settings.Validate();
	return true;
}

static void UnloadWorld(Struktur::GameContext& context)
{
	context.GetWorldRenderer().Shutdown();
	context.GetWorld().Unload();
}

// World.load(settingsJson) -> bool: builds the world and its GPU resources (replacing a loaded one). False when the
// settings disable the world, there is no biome data, or the argument isn't a settings string.
static void wren_WorldLoad(WrenVM* vm)
{
	Struktur::GameContext& context = ContextOf(vm);
	World::WorldSettings settings;
	const bool valid = SettingsFromSlot(vm, 1, settings);
	UnloadWorld(context);
	World::World& world                = context.GetWorld();
	const World::WorldAssets& assets   = world.GetAssets();
	if (!valid || !settings.enabled || !assets.biomes || assets.biomes->empty())
	{
		wrenSetSlotBool(vm, 0, false);
		return;
	}
	world.Load(settings, &context.GetJobSystem());
	if (Struktur::Core::CommandLine::HasFlag("--demo") && settings.demoPacer)
	{
		world.SetActorSource(std::make_unique<World::DemoPacer>());
	}
	context.GetWorldRenderer().Initialise(assets.manifest, assets.spriteDescriptor);
	wrenSetSlotBool(vm, 0, true);
}

// World.unload()
static void wren_WorldUnload(WrenVM* vm)
{
	UnloadWorld(ContextOf(vm));
}

// World.isLoaded -> bool
static void wren_WorldIsLoaded(WrenVM* vm)
{
	wrenSetSlotBool(vm, 0, ContextOf(vm).GetWorld().IsLoaded());
}

// World.applySettings(settingsJson) -> bool: changes a loaded world in place (geometry is rebuilt only if needed);
// settings that disable the world unload it. False when nothing is loaded or the argument isn't a settings string.
static void wren_WorldApplySettings(WrenVM* vm)
{
	Struktur::GameContext& context = ContextOf(vm);
	World::World& world            = context.GetWorld();
	World::WorldSettings settings;
	if (!world.IsLoaded() || !SettingsFromSlot(vm, 1, settings))
	{
		wrenSetSlotBool(vm, 0, false);
		return;
	}
	if (!settings.enabled)
	{
		UnloadWorld(context);
		wrenSetSlotBool(vm, 0, true);
		return;
	}
	const bool newRoad = world.GetSettings().seed != settings.seed || world.GetSettings().biome != settings.biome;
	world.SetSettings(settings);
	if (newRoad)
	{
		context.GetWorldRenderer().ResetChunks();
	}
	wrenSetSlotBool(vm, 0, true);
}

// World.biomes -> list of biome ids (assets/World/Biomes)
static void wren_WorldBiomes(WrenVM* vm)
{
	Struktur::Callback::VariantList list;
	const World::WorldAssets& assets = ContextOf(vm).GetWorld().GetAssets();
	if (assets.biomes)
	{
		for (const World::BiomeDefinition& biome : *assets.biomes)
		{
			list.items.push_back(biome.id);
		}
	}
	Struktur::Callback::HelperFunctions::VariantToWrenSlot(vm, 0, list);
}

// World.info -> map (biome, distance, grade) or null while nothing is loaded
static void wren_WorldInfo(WrenVM* vm)
{
	World::World& world = ContextOf(vm).GetWorld();
	if (!world.IsLoaded())
	{
		wrenSetSlotNull(vm, 0);
		return;
	}
	const World::BiomeBlend blend                     = world.GetBlend();
	const std::vector<World::BiomeDefinition>& biomes = world.GetBiomes();
	Struktur::Callback::VariantMap map;
	map.items["biome"]    = biomes.empty() ? std::string() : biomes[(size_t)blend.from].name;
	map.items["distance"] = world.GetDistance();
	map.items["grade"]    = world.GetRoadGradePercent();
	Struktur::Callback::HelperFunctions::VariantToWrenSlot(vm, 0, map);
}

// World.jumpTo(distance) - development helper: moves the rider along the same seeded road
static void wren_WorldJumpTo(WrenVM* vm)
{
	World::World& world = ContextOf(vm).GetWorld();
	if (world.IsLoaded() && wrenGetSlotType(vm, 1) == WREN_TYPE_NUM)
	{
		world.JumpTo(std::clamp(wrenGetSlotDouble(vm, 1), 0.0, 1.0e7));
	}
}

WREN_BINDING_MODULE(World)
{
	WREN_CLASS_STATIC(registry, "world", "World", "load(_)", wren_WorldLoad,
	                  "Loads the world (and its GPU resources) from a settings string - see Ride.worldSettings().");
	WREN_CLASS_STATIC(registry, "world", "World", "unload()", wren_WorldUnload,
	                  "Releases the loaded world and its GPU resources.");
	WREN_CLASS_STATIC(registry, "world", "World", "isLoaded", wren_WorldIsLoaded, "True while a world is loaded.");
	WREN_CLASS_STATIC(registry, "world", "World", "applySettings(_)", wren_WorldApplySettings,
	                  "Applies a changed settings string to the loaded world.");
	WREN_CLASS_STATIC(registry, "world", "World", "biomes", wren_WorldBiomes, "Biome ids from assets/World/Biomes.");
	WREN_CLASS_STATIC(registry, "world", "World", "info", wren_WorldInfo,
	                  "The loaded world's biome name, distance and road grade, or null.");
	WREN_CLASS_STATIC(registry, "world", "World", "jumpTo(_)", wren_WorldJumpTo,
	                  "Development: moves the rider to a distance (m) along the same seeded road.");
}
