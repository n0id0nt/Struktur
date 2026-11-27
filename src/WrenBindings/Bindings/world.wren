// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: world

class World {
    // Loads the world (and its GPU resources) from a settings string - see Ride.worldSettings().
    foreign static load(arg0)
    // Releases the loaded world and its GPU resources.
    foreign static unload()
    // True while a world is loaded.
    foreign static isLoaded
    // Applies a changed settings string to the loaded world.
    foreign static applySettings(arg0)
    // Biome ids from assets/World/Biomes.
    foreign static biomes
    // The loaded world's biome name, distance and road grade, or null.
    foreign static info
    // Development: moves the rider to a distance (m) along the same seeded road.
    foreign static jumpTo(arg0)
}

