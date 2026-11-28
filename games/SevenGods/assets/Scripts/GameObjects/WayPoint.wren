// GameObjects/WayPoint.wren
// One point of a route (LDtk "WayPoint" entity) - invisible, does nothing by itself. A Path (GameObjects/Path.wren)
// lists its WayPoints in order in its Entity_ref field; characters walk to each one's centre.
class WayPoint {
    construct new(entity, args) {
        _entity = entity
        _iid = args["Iid"]
    }

    iid { _iid }

    start() {}

    update() {}

    onDestroy() {}

    onEvent(type, data) {}

    getEntity() { _entity }
}
