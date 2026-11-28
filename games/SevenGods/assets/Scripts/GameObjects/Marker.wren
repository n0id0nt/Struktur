// GameObjects/Marker.wren
// A named spot in the level for dialogue staging (LDtk "Marker" entity) - where the "moveTo" / "teleport" dialogue
// commands send a character (World/StageDirector.wren). Invisible; does nothing by itself.
//   Name   (String)            - what the dialogue refers to it by, e.g. "FinnAtAnvil". Several markers can share a
//                                name (one per room) - the one nearest the character is used.
//   Facing (Directions, opt.)  - the way a character arriving here ends up facing (North / South / East / West).
class Marker {
    construct new(entity, args) {
        _entity = entity
        _name = args["Name"]
        _facing = args["Facing"]
        if (_facing == "") {
            _facing = null
        }
    }

    name { _name }
    // "North" / "South" / "East" / "West", or null.
    facing { _facing }

    start() {}

    update() {}

    onDestroy() {}

    onEvent(type, data) {}

    getEntity() { _entity }
}
