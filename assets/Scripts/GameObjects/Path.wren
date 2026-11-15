// GameObjects/Path.wren
// A walking route (LDtk "Path" entity): the WayPoints its Entity_ref field lists, in that order. Invisible.
//   Name           (String)                 - what dialogue calls it ("moveTo" / "teleport" - World/StageDirector.wren).
//   Entity_ref     (Array of WayPoint refs) - the points, in walking order.
//   PathFollowType (Once / Loop / PingPong / Random) - how a wandering NPC walks it: once through and stop, round and
//                                             round (last point back to the first), there and back again, or from
//                                             point to random point. A dialogue scene always walks it once, in order.
// Every Path in the loaded rooms is listed in World/Paths.wren. An NPC wanders the Path its own Entity_ref field
// points at (World/PathWalkers.wren).
import "gameObject" for GameObject
import "gameObjectComponents" for Script, WorldTransform
import "math" for Vec2
import "debug" for Debug
import "World/Paths" for Paths

class Path {
    construct new(entity, args) {
        _entity = entity
        _iid = args["Iid"]
        _name = args["Name"]
        _followType = args["PathFollowType"] == null ? "Once" : args["PathFollowType"]
        _iids = []
        var refs = args["Entity_ref"]
        if (refs is String && refs != "") {
            _iids = refs.split(",")
        }
        _points = null   // resolved on first use - see points
    }

    iid { _iid }
    name { _name }
    followType { _followType }
    entity { _entity }

    // The world positions (Vec2) of its WayPoints in walking order - the ones that are loaded (a Path's WayPoints are
    // in its own level, so they load with it).
    points {
        if (_points != null) return _points
        var byIid = {}
        for (wayPoint in GameObject.getAllWithIdentifier("WayPoint")) {
            var script = Script.getInstance(wayPoint)
            if (script != null) byIid[script.iid] = wayPoint
        }
        var found = []
        for (iid in _iids) {
            var wayPoint = byIid[iid]
            if (wayPoint != null) {
                var p = WorldTransform.getPosition(wayPoint)
                found.add(Vec2.new(p.x, p.y))
            }
        }
        // Only remembered once every point is there (a WayPoint's script may not have started yet on the first frame).
        if (found.count == _iids.count) _points = found
        return found
    }

    start() {
        Paths.register(_entity)
    }

    update() {}

    onDestroy() {
        Paths.unregister(_entity)
    }

    onEvent(type, data) {}

    getEntity() { _entity }
}
