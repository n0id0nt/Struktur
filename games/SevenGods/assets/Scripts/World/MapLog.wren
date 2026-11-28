// World/MapLog.wren
// What the player has explored, for the inventory's Map tab (UI/Inventory/MapPage.wren): every mappable room walked
// into, and every doorway taken into an interior. Saved to map.sav like the inventory (loaded in Main.wren).
//
// Each LDtk level's "Mappable" field (the Mappable enum) says how it shows:
//   Mappable  - drawn as its own rectangle once visited (the default for a level without the field);
//   WorpPoint - an interior (a house, a cabin): never drawn itself, but the warp door the player took to get in
//               shows on the map as a doorway marker in the room outside;
//   No        - never on the map (the battle arenas).
// States/GameWorldState records rooms as the player enters them (visit, from updateLevelStreaming_) and doorways as
// they warp (warped, from updateWarp_).
//
// rooms() turns the log into what the map draws: each visited room's world rectangle and the colour of its Terrain
// (the LDtk level field) - the BattleArena enum's colour for that value, read straight from the LDtk file
// (World.getLevelFieldColor), so recolouring a terrain in LDtk recolours the map. doors() lists the doorways.
import "serialisation" for Json, MapUtil
import "fileSystem" for FileSystem
import "gameObjectComponents" for World

var SAVE_PATH = "map.sav"
var TERRAIN_FIELD = "Terrain"
var MAPPABLE_FIELD = "Mappable"
var MAPPABLE = "Mappable"
var WARP_POINT = "WorpPoint"
var NOT_MAPPABLE = "No"
var ARENA_SUFFIX = "BattleStage"         // a level with no Mappable field: arenas still never count
var NO_TERRAIN_COLOR = [150, 150, 150]   // a room with no Terrain set

class MapLog {
    static init() {
        __visited = []
        __doors = []
        __current = null
    }

    static visited { __visited }

    static hasVisited(identifier) { __visited.contains(identifier) }

    // The room the player is in (its identifier - an interior's too), or null before gameplay has placed them.
    static current { __current }

    // How a level shows on the map: MAPPABLE, WARP_POINT or NOT_MAPPABLE (see the top of this file).
    static kind(world, levelIndex) {
        var value = World.getLevelField(world, levelIndex, MAPPABLE_FIELD)
        if (value == MAPPABLE || value == WARP_POINT || value == NOT_MAPPABLE) return value
        var identifier = World.getLevelIdentifier(world, levelIndex)
        return (identifier != null && identifier.endsWith(ARENA_SUFFIX)) ? NOT_MAPPABLE : MAPPABLE
    }

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

    // The player is in room `levelIndex` of `world` (the world entity): logs a mappable room the first time. An
    // interior becomes the current room but is never logged; an unmappable level changes nothing.
    static visit(world, levelIndex) {
        if (world == null || levelIndex == null) return
        var identifier = World.getLevelIdentifier(world, levelIndex)
        if (identifier == null) return
        var kind = MapLog.kind(world, levelIndex)
        if (kind == NOT_MAPPABLE) return
        __current = identifier
        if (kind != MAPPABLE || __visited.contains(identifier)) return
        __visited.add(identifier)
        MapLog.save(SAVE_PATH)
    }

    // The party warped from room fromIndex to targetIndex. Going into an interior (WorpPoint) from a mappable room
    // logs the doorway where the player stepped on it (origin); coming out of one logs it where they arrive outside
    // (arrival) - either way it's the door between the two. One doorway per interior per outside room.
    static warped(world, fromIndex, targetIndex, origin, arrival) {
        if (world == null || fromIndex == null || targetIndex == null) return
        var fromKind = MapLog.kind(world, fromIndex)
        var targetKind = MapLog.kind(world, targetIndex)
        if (targetKind == WARP_POINT && fromKind == MAPPABLE && origin != null) {
            MapLog.addDoor_(World.getLevelIdentifier(world, targetIndex), World.getLevelIdentifier(world, fromIndex), origin)
        } else if (fromKind == WARP_POINT && targetKind == MAPPABLE && arrival != null) {
            MapLog.addDoor_(World.getLevelIdentifier(world, fromIndex), World.getLevelIdentifier(world, targetIndex), arrival)
        }
    }

    static addDoor_(interior, room, position) {
        if (interior == null || room == null) return
        for (door in __doors) {
            if (door[0] == interior && door[1] == room) return
        }
        __doors.add([interior, room, position.x.round, position.y.round])
        MapLog.save(SAVE_PATH)
    }

    // Every visited mappable room still in the world:
    //   [{ "id", "bounds": [x, y, w, h] (world px), "color": [r, g, b], "terrain": its Terrain value or "" }].
    static rooms(world) {
        var rooms = []
        if (world == null) return rooms
        for (identifier in __visited) {
            var index = World.getLevelIndex(world, identifier)
            // Re-checked here too, so a room since marked No / WorpPoint in LDtk drops off older saves' maps.
            if (index != null && index >= 0 && MapLog.kind(world, index) == MAPPABLE) {
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

    // Every doorway taken into an interior whose outside room is on the map:
    //   [{ "interior": the interior's identifier, "room": the outside room's, "position": [x, y] (world px) }].
    static doors(world) {
        var doors = []
        for (door in __doors) {
            if (__visited.contains(door[1])) {
                var entry = {}
                entry["interior"] = door[0]
                entry["room"] = door[1]
                entry["position"] = [door[2], door[3]]
                doors.add(entry)
            }
        }
        return doors
    }

    // --- Serialisation ---

    static save(path) {
        var json = Json.stringify(MapUtil.mapToPairs({"visited": __visited, "doors": __doors}))
        var result = FileSystem.writeString(path, json)
        if (result.success) {
            FileSystem.syncSaves()
        }
    }

    static load(path) {
        __visited = []
        __doors = []
        var result = FileSystem.readString(path)
        if (result.success) {
            var data = Json.parse(result.value)
            var loaded = data == null ? null : data["visited"]
            if (loaded != null) {
                for (identifier in loaded) {
                    __visited.add(identifier)
                }
            }
            var doors = data == null ? null : data["doors"]
            if (doors != null) {
                for (door in doors) {
                    if (door is List && door.count == 4) __doors.add(door)
                }
            }
        }
    }
}
