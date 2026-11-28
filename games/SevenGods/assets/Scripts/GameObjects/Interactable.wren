// GameObjects/Interactable.wren
// Something built into the level that can be examined or talked to - a tablet, a mural, a crystal, the Wardens
// (LDtk "Interactable" entity; the art is part of the level's tiles, so this draws nothing and has no body).
//   MessageKey - its name: the key Dialogue/DialogueEntryPoints.wren looks its conversation up by. With no entry
//                there, it's read as a Localization key (en.json) for a single line instead.
// Several Interactables can share a MessageKey (the three Wardens) - a "startDuel" with that name fights them all at
// once (World/Npcs.wren, States/GameWorldState.startDuel).
import "localization" for Localization
import "World/StoryGates" for StoryGates

class Interactable {
    construct new(entity, args) {
        _entity = entity
        // Its LDtk size - Player.clearLineTo_ ignores hits inside it (it's usually built into solid tiles).
        _size = [args["Width"] != null ? args["Width"] : 16, args["Height"] != null ? args["Height"] : 16]
        _name = args["MessageKey"]
        _gate = StoryGates.fromArgs(args)
    }

    name { _name }

    start() {
        StoryGates.register(_entity, _gate)
    }

    update() {}

    onDestroy() {
        StoryGates.unregister(_entity)
    }

    onEvent(type, data) {}

    // Read with no dialogue of its own (States/InteractState.wren): its message line.
    interact() {
        if (_name == null || _name == "") return null
        return Localization.get(_name)
    }

    isInteractable() { StoryGates.isVisible(_entity) }

    requiresFacing() { true }

    // [width, height] px - see Player.clearLineTo_.
    interactSize() { _size }

    getEntity() { _entity }
}
