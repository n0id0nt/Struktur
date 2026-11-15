// World/MapLog.wren
// Every level (room) the player has walked into, by its LDtk identifier, for the inventory's Map tab
// (UI/Inventory/MapPage.wren). States/GameWorldState.updateLevelStreaming_ records each room as the player enters it;
// saved to map.sav like the inventory (loaded in Main.wren).
//
// rooms() turns the log into what the map draws: each visited room's world rectangle and the colour of its Terrain
// (the LDtk level field) - the BattleArena enum's colour for that value, read straight from the LDtk file
// (World.getLevelFieldColor), so recolouring a terrain in LDtk recolours the map. Battle arenas never count.
import "serialisation" for Json, MapUtil
import "fileSystem" for FileSystem
import "gameObjectComponents" for World

var SAVE_PATH = "map.sav"
var TERRAIN_FIELD = "Terrain"
var ARENA_SUFFIX = "BattleStage"
var NO_TERRAIN_COLOR = [150, 150, 150]   // a room with no Terrain set

class MapLog {
    static init() {
        __visited = []
    }

    static visited { __visited }

    static hasVisited(identifier) { __visited.contains(identifier) }

    // The room the player is in (its identifier), or null before gameplay has placed them.
    static current { __current }

    // "HavenVillageBorderNorth" -> "Haven Village Border North": a room's identifier as display text.
    static displayName(identifier) {
        if (identifier == null) return null
        var text = ""
        for (i in 0...identifier.count) {
            var ch = identifier[i]
            var code = ch.bytes[0]
            var upper = code >= 65 && code <= 90
            var digit = code >= 48 && code <= 57
            if (i > 0 && (upper || (digit && !MapLog.isDigit_(identifier[i - 1])))) text = text + " "
            text = text + ch
        }
        return text
    }

    static isDigit_(ch) { ch.bytes[0] >= 48 && ch.bytes[0] <= 57 }

    // The player is in room `levelIndex` of `world` (the world entity): logs it the first time.
    static visit(world, levelIndex) {
        if (world == null || levelIndex == null) return
        var identifier = World.getLevelIdentifier(world, levelIndex)
        if (identifier == null || identifier.endsWith(ARENA_SUFFIX)) return
        __current = identifier
        if (__visited.contains(identifier)) return
        __visited.add(identifier)
        MapLog.save(SAVE_PATH)
    }

    // Every visited room still in the world:
    //   [{ "id", "bounds": [x, y, w, h] (world px), "color": [r, g, b], "terrain": its Terrain value or "" }].
    static rooms(world) {
        var rooms = []
        if (world == null) return rooms
        for (identifier in __visited) {
            var index = World.getLevelIndex(world, identifier)
            if (index != null && index >= 0) {
                var color = World.getLevelFieldColor(world, index, TERRAIN_FIELD)
                var room = {}
                room["id"] = identifier
                room["bounds"] = World.getLevelBounds(world, index)
                room["color"] = color != null ? color : NO_TERRAIN_COLOR
                var terrain = World.getLevelField(world, index, TERRAIN_FIELD)
                room["terrain"] = terrain is String ? terrain : ""
                rooms.add(room)
            }
        }
        return rooms
    }

    // --- Serialisation ---

    static save(path) {
        var json = Json.stringify(MapUtil.mapToPairs({"visited": __visited}))
        var result = FileSystem.writeString(path, json)
        if (result.success) {
            FileSystem.syncSaves()
        }
    }

    static load(path) {
        __visited = []
        var result = FileSystem.readString(path)
        if (result.success) {
            var data = Json.parse(result.value)
            var loaded = data == null ? null : data["visited"]
            if (loaded != null) {
                for (identifier in loaded) {
                    __visited.add(identifier)
                }
            }
        }
    }
}
