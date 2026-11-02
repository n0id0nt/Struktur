// World/Rooms.wren
// The rooms of the current LDtk world - each level is a room. Answers "which room is this position in?" and
// "what are this room's bounds?" straight from the parsed LDtk metadata (World.getLevelBounds), so it works for
// rooms that aren't loaded. The gameplay state (States/ExperimentState.wren) sets the world on enter and clears
// it on exit; critters use it to stay in their home room (GameObjects/Critter.wren), and the camera and level
// streaming use it to follow the player between rooms.
import "gameObjectComponents" for World

class Rooms {
    static world { __world }
    static world=(value) { __world = value }

    // The index of the room whose bounds contain position (a Vec2/Vec3), or null if none does or there's no
    // world. Rooms only meet at their edges, so a position right on an edge counts for the first one listed.
    static indexAt(position) {
        if (__world == null || position == null) {
            return null
        }
        for (i in 0...World.getLevelsCount(__world)) {
            if (Rooms.contains(i, position, 0)) {
                return i
            }
        }
        return null
    }

    // [x, y, width, height] of the room in world px, or null.
    static bounds(index) {
        if (__world == null || index == null) {
            return null
        }
        return World.getLevelBounds(__world, index)
    }

    // True if position is inside the room, at least `inset` px in from every edge.
    static contains(index, position, inset) {
        var b = Rooms.bounds(index)
        if (b == null || position == null) {
            return false
        }
        return position.x >= b[0] + inset && position.x <= b[0] + b[2] - inset &&
               position.y >= b[1] + inset && position.y <= b[1] + b[3] - inset
    }
}
