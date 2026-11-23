// GameObjects/DoorWorp.wren
// A doorway to another room - e.g. a tent's entrance. Any part of the player's collider (Util/CharacterCollider.wren -
// not followers) over its trigger warps the party to the DoorWorp its "Destination" field links to. It checks the
// overlap itself every frame rather than waiting for physics contact events, so it can't miss a player who steps
// straight back on after arriving, or keep a stale "inside" from before a teleport. Invisible - the doorway itself is drawn by the level's tiles. The move is done by
// States/GameWorldState.wren (via World/Warps.wren). Placed in the level (LDtk "DoorWorp" entity).
//
// A door with a direction field (where you stand arriving through it - North = above it) is only a trigger on its far
// half: a North door's trigger is the bottom half of its rect, so the trigger's edge runs through the door's centre.
// Arrivals stand arrivalDistance from the centre in that direction - the same distance whichever way the door faces,
// and just far enough that the player's collider (Util/CharacterCollider.wren) is clear of the trigger. A door with
// no direction keeps its whole rect, and arrivals step a little further, towards the room's centre.
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, World
import "math" for Vec2
import "app" for Application
import "debug" for Debug
import "World/Rooms" for Rooms
import "World/Warps" for Warps
import "Util/CharacterCollider" for CharacterCollider

// The trigger's size in px when there's no LDtk size (a script-spawned DoorWorp) - one tile.
var DEFAULT_SIZE = 16
// Px past the collider's reach that an arrival stands clear of the trigger's edge.
var ARRIVAL_MARGIN = 1

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
        _direction = DoorWorp.directionFrom_(args)
        _arrivalOffset = null
        if (_direction != null) {
            var distance = DoorWorp.arrivalDistance
            _arrivalOffset = Vec2.new(_direction.x * distance, _direction.y * distance)
        }
        _targetIndex = null
        // The trigger, in px: its centre's offset from the door's centre, and its half-size (set in start).
        _triggerOffset = Vec2.new(0, 0)
        _triggerHalf = Vec2.new(_width / 2, _height / 2)
    }

    // Px from a door's centre that someone arriving through it stands, in every direction: the trigger's edge runs
    // through the centre, so the feet must be far enough out that no part of the collider - a circle raised above the
    // feet - is still over it. The worst case (arriving south, the circle reaching back up towards the door) sets it.
    static arrivalDistance {
        var ppm = Application.pixelsPerMeter
        return (CharacterCollider.radius * ppm + CharacterCollider.offsetY.abs * ppm + ARRIVAL_MARGIN).ceil
    }

    static nonEmpty_(value) {
        if (value == null || value == "") return null
        return value
    }

    // The first string arg whose name contains "direction" (any case), as a unit step (x / y each -1, 0 or 1) - or null.
    static directionFrom_(args) {
        for (key in args.keys) {
            var value = args[key]
            if (value is String && (key.contains("Direction") || key.contains("direction"))) {
                var direction = DoorWorp.stepFor_(value)
                if (direction == null) Debug.warning("DoorWorp: unknown direction '%(value)'")
                return direction
            }
        }
        return null
    }

    static stepFor_(direction) {
        var heading = direction
        var x = 0
        var y = 0
        if (heading.contains("North") || heading.contains("north") || heading.contains("Up") || heading.contains("up")) y = -1
        if (heading.contains("South") || heading.contains("south") || heading.contains("Down") || heading.contains("down")) y = 1
        if (heading.contains("East") || heading.contains("east") || heading.contains("Right") || heading.contains("right")) x = 1
        if (heading.contains("West") || heading.contains("west") || heading.contains("Left") || heading.contains("left")) x = -1
        if (x == 0 && y == 0) return null
        return Vec2.new(x, y)
    }

    name { "DoorWorp" }
    iid { _iid }
    // The room index it leads to, or null (no destination / no such level) - known once started.
    targetIndex { _targetIndex }
    // Px offset from this door's centre where someone arriving through it stands, or null if it has no direction.
    arrivalOffset { _arrivalOffset }
    getEntity() { _entity }

    start() {
        // With a direction, only the half of the rect away from it is the trigger (see the header).
        var halfWidth = _width / 2
        var halfHeight = _height / 2
        var centreX = 0
        var centreY = 0
        if (_direction != null && _direction.x != 0) {
            halfWidth = _width / 4
            centreX = -_direction.x * _width / 4
        }
        if (_direction != null && _direction.y != 0) {
            halfHeight = _height / 4
            centreY = -_direction.y * _height / 4
        }
        _triggerOffset = Vec2.new(centreX, centreY)
        _triggerHalf = Vec2.new(halfWidth, halfHeight)

        if (Rooms.world != null && _level != null) {
            _targetIndex = World.getLevelIndex(Rooms.world, _level)
        }
        if (_targetIndex == null) {
            Debug.warning("DoorWorp %(_iid): no destination - link its Destination to another DoorWorp in LDtk")
        }
    }

    // Every frame the player's collider is over the trigger, it asks for the warp (the gameplay state ignores the request
    // while a warp is already under way, and takes it as soon as it can).
    update() {
        if (_targetIndex == null || !playerOverlaps_) return
        Warps.request(_targetIndex, Rooms.indexAt(WorldTransform.getPosition(_entity)), _destination)
    }

    // Whether the player's collider - a circle raised above the feet - touches the trigger rect at all.
    playerOverlaps_ {
        var players = GameObject.getAllWithIdentifier("Player")
        if (players.count == 0) return false
        var leader = players[0]
        var ppm = Application.pixelsPerMeter
        var feet = WorldTransform.getPosition(leader)
        var circleX = feet.x
        var circleY = feet.y + CharacterCollider.offsetY * ppm
        var radius = CharacterCollider.radius * ppm
        var door = WorldTransform.getPosition(_entity)
        var rectX = door.x + _triggerOffset.x
        var rectY = door.y + _triggerOffset.y
        // The rect's nearest point to the circle's centre, then whether that is within the radius.
        var nearestX = circleX.max(rectX - _triggerHalf.x).min(rectX + _triggerHalf.x)
        var nearestY = circleY.max(rectY - _triggerHalf.y).min(rectY + _triggerHalf.y)
        var dx = circleX - nearestX
        var dy = circleY - nearestY
        return dx * dx + dy * dy < radius * radius
    }

    onDestroy() {}

    onEvent(type, data) {}

    isInteractable() { false }
}
