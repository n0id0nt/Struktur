// GameObjects/Level/Critter.wren
// The LDtk "Critter" entity: a spawner, not a creature. Its "Enemy" dropdown (Chinlin / Chicken) picks the species,
// and on start it creates that creature (GameObjects/Chinlin.wren, GameObjects/Chicken.wren) where it stands, passing
// its own iid on so a defeated one stays gone (Combat/Defeated.wren). The creature is its child, so it unloads with
// the level.
// Registered as the "Critter" script (Main.wren). Kept in its own folder because a script's module is named after its
// file, and GameObjects/Critter.wren is the shared base class every creature extends - a different thing.
import "gameObject" for GameObject
import "gameObjectComponents" for Script, WorldTransform
import "debug" for Debug

// The Enemy values that have a creature script.
var SPECIES = ["Chinlin", "Chicken"]

class Critter {
    construct new(entity, args) {
        _entity = entity
        _iid = args["Iid"]
        _species = args["Enemy"]
    }

    start() {
        if (!SPECIES.contains(_species)) {
            Debug.warning("[Critter] %(_iid): Enemy '%(_species)' has no creature script - nothing spawned")
            return
        }
        var creature = GameObject.create(_species, _entity)
        // Name picks its battle stats (Combat/Config/BattleCritter.forName).
        Script.createArg(creature, _species, {"Name": _species, "Iid": _iid})
        WorldTransform.setPosition(creature, WorldTransform.getPosition(_entity))
    }

    update() {}

    onDestroy() {}

    onEvent(type, data) {}

    getEntity() { _entity }
}
