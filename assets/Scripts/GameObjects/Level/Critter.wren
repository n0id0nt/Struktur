// GameObjects/Level/Critter.wren
// The LDtk "Critter" entity: a spawner, not a creature. Its "Enemy" dropdown picks the species, and on start it creates
// that creature (GameObjects/Chinlin.wren, GameObjects/Chicken.wren) where it stands, passing its own iid on so a
// defeated one stays gone (Combat/Defeated.wren). The creature is its child, so it unloads with the level.
// A species tied to a quest (the perimeter wolf Thomas sends you after) only appears while that quest is active: on
// start if it already is, or on the questStarted event (Events/GameEvents.wren) if the level is loaded when it starts.
// Registered as the "Critter" script (Main.wren). Kept in its own folder because a script's module is named after its
// file, and GameObjects/Critter.wren is the shared base class every creature extends - a different thing.
import "gameObject" for GameObject
import "gameObjectComponents" for Script, WorldTransform
import "debug" for Debug
import "Story/QuestLog" for QuestLog
import "Events/GameEvents" for GameEvents

class Critter {
    // Enemy value -> [creature script, name (picks its battle def - Combat/Config/BattleCritter.forName), quest it
    // waits for or null]. The wolf borrows the Chinlin's script and look until it has its own.
    static species_ {
        if (__species == null) {
            __species = {
                "Chinlin": ["Chinlin", "Chinlin", null],
                "Chicken": ["Chicken", "Chicken", null],
                "StarvingThornwolf": ["Chinlin", "Starving Thornwolf", null],
                // The one at the village's north edge that Thomas sends you after.
                "PerimeterThornwolf": ["Chinlin", "Starving Thornwolf", "wolf_at_the_edge"]
            }
        }
        return __species
    }

    construct new(entity, args) {
        _entity = entity
        _iid = args["Iid"]
        _species = args["Enemy"]
        _spawned = false
    }

    start() {
        var entry = Critter.species_[_species]
        if (entry == null) {
            Debug.warning("[Critter] %(_iid): Enemy '%(_species)' has no creature script - nothing spawned")
            return
        }
        if (entry[2] == null || QuestLog.status(entry[2])["state"] == QuestLog.active) spawn_(entry)
    }

    spawn_(entry) {
        if (_spawned) return
        _spawned = true
        var creature = GameObject.create(entry[0], _entity)
        Script.createArg(creature, entry[0], {"Name": entry[1], "Iid": _iid})
        WorldTransform.setPosition(creature, WorldTransform.getPosition(_entity))
    }

    update() {}

    onDestroy() {}

    onEvent(type, data) {
        if (type != GameEvents.questStartedType || _spawned) return
        var entry = Critter.species_[_species]
        if (entry != null && entry[2] != null && data["quest"] == entry[2]) spawn_(entry)
    }

    getEntity() { _entity }
}
