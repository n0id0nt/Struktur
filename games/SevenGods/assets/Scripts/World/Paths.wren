// World/Paths.wren
// Every Path (GameObjects/Path.wren - LDtk "Path" entity) in the loaded rooms. Paths add themselves when their level
// loads and drop out when it unloads. Dialogue scenes find one by name (World/StageDirector.wren); wandering NPCs are
// handed theirs by World/PathWalkers.wren.
import "gameObject" for GameObject
import "gameObjectComponents" for Script, WorldTransform
import "math" for Vec3

class Paths {
    static entities_ {
        if (__entities == null) __entities = []
        return __entities
    }

    static register(entity) {
        if (!Paths.entities_.contains(entity)) Paths.entities_.add(entity)
    }

    static unregister(entity) {
        Paths.entities_.remove(entity)
    }

    // The scripts of every loaded Path (GameObjects/Path.wren).
    static all {
        var scripts = []
        for (entity in Paths.entities_) {
            if (!GameObject.isValid(entity)) continue
            var script = Script.getInstance(entity)
            if (script != null) scripts.add(script)
        }
        return scripts
    }

    // The loaded Path called `name` whose first point is nearest `near` (a position, or null), or null.
    static named(name, near) {
        var best = null
        var bestDistance = null
        for (path in Paths.all) {
            if (path.name != name) continue
            var points = path.points
            if (points.count == 0) continue
            var distance = near == null ? 0 : Vec3.distance(Vec3.new(points[0].x, points[0].y, 0), near)
            if (best == null || distance < bestDistance) {
                best = path
                bestDistance = distance
            }
        }
        return best
    }
}
