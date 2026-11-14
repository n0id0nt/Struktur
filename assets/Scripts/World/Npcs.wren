// World/Npcs.wren
// Finds the standing characters called `name`. A script-spawned NPC's entity identifier is its name; an NPC placed in
// the level (LDtk "NPC" entity, Name field) or a character built into the level (LDtk "Interactable" entity, its
// MessageKey - e.g. the three Wardens) only has the name on its script - so all of those are searched.
// Used by the joinParty dialogue command and duels (States/GameWorldState.startDuel - every match fights together).
import "gameObject" for GameObject
import "gameObjectComponents" for Script
import "World/StoryGates" for StoryGates

// Level entity identifiers whose script carries a character's name.
var LEVEL_IDENTIFIERS = ["NPC", "Interactable"]

class Npcs {
    // Every entity standing in for `name` (hidden story-gated copies are skipped).
    static find(name) {
        var found = []
        for (entity in GameObject.getAllWithIdentifier(name)) {
            found.add(entity)
        }
        for (identifier in LEVEL_IDENTIFIERS) {
            for (entity in GameObject.getAllWithIdentifier(identifier)) {
                var script = Script.getInstance(entity)
                if (script != null && script.name == name && StoryGates.isVisible(entity)) {
                    found.add(entity)
                }
            }
        }
        return found
    }

    static first(name) {
        var found = Npcs.find(name)
        return found.count > 0 ? found[0] : null
    }
}
