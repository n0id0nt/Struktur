// GameObjects/Spawn.wren
// A spot the player can start from / wake up at (LDtk "Spawn" entity). Invisible and does nothing by itself: the
// gameplay state (States/GameWorldState.wren) streams in the level of the current spawn (World/SpawnPoint.wren) and
// moves the party onto the Spawn whose iid matches. A RestSpot's "SpawnPoint" field links it to one of these.
class Spawn {
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
