// Defeated.wren
// Every world opponent that has been killed for good, by its LDtk instance id (the "Iid" arg the engine hands
// every level entity's script - see src/Engine/World/Level.cpp). Level streaming re-creates a level's entities
// each time it loads, so without this a killed critter would be back the next time you walked into its level;
// with it, a Critter whose iid is listed removes itself as it spawns (GameObjects/Critter.wren).
// Recorded by States/Combat/CombatResolver.removeFallen; saved to defeated.sav like the inventory. Opponents
// spawned from script (the NPCs) have no iid and aren't tracked here - their own spawn logic decides whether
// they appear (e.g. the Duelist only stands in the world until Party.contains it).
import "serialisation" for Json, MapUtil
import "fileSystem" for FileSystem

var SAVE_PATH = "defeated.sav"

class Defeated {
    static init() {
        __iids = []
    }

    static iids { __iids }

    // True when the opponent with this iid has been killed (false for a null iid - a script-spawned entity).
    static contains(iid) { iid != null && __iids.contains(iid) }

    // Records a kill and saves straight away (no-op for a null or already-recorded iid).
    static add(iid) {
        if (iid == null || __iids.contains(iid)) {
            return
        }
        __iids.add(iid)
        Defeated.save(SAVE_PATH)
    }

    // --- Serialisation ---

    static save(path) {
        var json = Json.stringify(MapUtil.mapToPairs({"iids": __iids}))
        var result = FileSystem.writeString(path, json)
        if (result.success) {
            FileSystem.syncSaves()
        }
    }

    static load(path) {
        __iids = []
        var result = FileSystem.readString(path)
        if (result.success) {
            var loaded = Json.parse(result.value)["iids"]
            if (loaded != null) {
                for (iid in loaded) {
                    __iids.add(iid)
                }
            }
        }
    }
}
