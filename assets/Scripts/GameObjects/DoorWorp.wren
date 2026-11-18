// GameObjects/DoorWorp.wren
// A doorway to another room - e.g. a tent's entrance. Stepping on it (the player only, not followers) warps the party
// to the DoorWorp its "Destination" field links to, arriving one tile from that door's centre in the direction the
// destination door's direction field gives (North = the tile above it, and so on). A destination with no direction
// set arrives one step towards its room's centre instead. Invisible - the doorway itself is drawn by the level's
// tiles. The move is done by States/GameWorldState.wren (via World/Warps.wren). Placed in the level (LDtk
// "DoorWorp" entity).
import "gameObjectComponents" for PhysicsBody, WorldTransform, World
import "physics" for BodyDefinition, PhysicsBoxShape, BodyType, CollisionLayers
import "math" for Vec2
import "app" for Application
import "debug" for Debug
import "Party/Party" for Party
import "World/Rooms" for Rooms
import "World/Warps" for Warps

// The trigger's size in px when there's no LDtk size (a script-spawned DoorWorp) - one tile.
var DEFAULT_SIZE = 16
// One tile, in px - how far from a destination door's centre you arrive.
var TILE_SIZE = 16

class DoorWorp {
    // args (from LDtk): "Iid"; "Destination" - the iid of the DoorWorp it leads to, with "DestinationLevels" (filled in
    // by World::Level) the room that one is in; a direction field - any field whose name contains "Direction" - where
    // to stand when arriving through this door: North / South / East / West (also Up / Down / Right / Left).
    // Older maps: "Level" - the room to go to, arriving next to the DoorWorp there that leads back. "Width" / "Height"
    // (px, from World::Level): the trigger covers exactly the entity's rect - e.g. a 32 x 16 tent doorway.
    construct new(entity, args) {
        _entity = entity
        _iid = args["Iid"]
        _width = args["Width"] != null ? args["Width"] : DEFAULT_SIZE
        _height = args["Height"] != null ? args["Height"] : DEFAULT_SIZE
        _destination = DoorWorp.nonEmpty_(args["Destination"])
        _level = DoorWorp.nonEmpty_(args["DestinationLevels"])
        if (_level == null) _level = DoorWorp.nonEmpty_(args["Level"])
        _arrivalOffset = DoorWorp.arrivalOffsetFrom_(args)
        _targetIndex = null
    }

    static nonEmpty_(value) {
        if (value == null || value == "") return null
        return value
    }

    // The first string arg whose name contains "direction" (any case), as a one-tile offset in px - or null.
    static arrivalOffsetFrom_(args) {
        for (key in args.keys) {
            var value = args[key]
            if (value is String && (key.contains("Direction") || key.contains("direction"))) {
                var offset = DoorWorp.offsetFor_(value)
                if (offset == null) Debug.warning("DoorWorp: unknown direction '%(value)'")
                return offset
            }
        }
        return null
    }

    static offsetFor_(direction) {
        var heading = direction
        var x = 0
        var y = 0
        if (heading.contains("North") || heading.contains("north") || heading.contains("Up") || heading.contains("up")) y = -1
        if (heading.contains("South") || heading.contains("south") || heading.contains("Down") || heading.contains("down")) y = 1
        if (heading.contains("East") || heading.contains("east") || heading.contains("Right") || heading.contains("right")) x = 1
        if (heading.contains("West") || heading.contains("west") || heading.contains("Left") || heading.contains("left")) x = -1
        if (x == 0 && y == 0) return null
        return Vec2.new(x * TILE_SIZE, y * TILE_SIZE)
    }

    name { "DoorWorp" }
    iid { _iid }
    // The room index it leads to, or null (no destination / no such level) - known once started.
    targetIndex { _targetIndex }
    // Px offset from this door's centre where someone arriving through it stands, or null if it has no direction.
    arrivalOffset { _arrivalOffset }
    getEntity() { _entity }

    start() {
        // Half-extents in metres.
        var ppm = Application.pixelsPerMeter
        var body = PhysicsBody.create(_entity, BodyDefinition.new(BodyType.STATIC_BODY),
                                      PhysicsBoxShape.new(_width / 2 / ppm, _height / 2 / ppm))
        body.isSensor = true
        body.setCollisionFilter(CollisionLayers.getLayer("Trigger"), CollisionLayers.getLayer("Actor"))

        if (Rooms.world != null && _level != null) {
            _targetIndex = World.getLevelIndex(Rooms.world, _level)
        }
        if (_targetIndex == null) {
            Debug.warning("DoorWorp %(_iid): no destination - link its Destination to another DoorWorp in LDtk")
        }
    }

    update() {}

    onDestroy() {}

    // Contact events go to every script, so only the ones involving this trigger count.
    onEvent(type, data) {
        if (type != "collisionEnter" || _targetIndex == null) {
            return
        }
        var other = null
        if (data["entityA"] == _entity) {
            other = data["entityB"]
        } else if (data["entityB"] == _entity) {
            other = data["entityA"]
        }
        if (other == null) {
            return
        }
        var member = Party.memberForEntity(other)
        if (member == null || !member.isPlayer) {
            return
        }
        Warps.request(_targetIndex, Rooms.indexAt(WorldTransform.getPosition(_entity)), _destination)
    }

    isInteractable() { false }
}
