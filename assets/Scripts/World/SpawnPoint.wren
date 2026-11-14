// World/SpawnPoint.wren
// Where the player starts a session and wakes up after the party is wiped: an LDtk "Spawn" entity, remembered by its
// iid (and the level it's in, so that level can be streamed in to find it). Saved in flags.sav, so it survives a
// restart. Resting at a RestSpot (GameObjects/RestSpot.wren) makes that rest spot's linked Spawn (its "SpawnPoint"
// field) the current one. A new game starts at DEFAULT_IID - the Spawn next to the bed in Finn's workshop.
import "flags" for FlagManager

var DEFAULT_IID = "2f3bf788-bde0-11f1-99f1-4379b4e07faa"
var DEFAULT_LEVEL = "FinnsWorkshop"

var IID_FLAG = "spawn_point_iid"
var LEVEL_FLAG = "spawn_point_level"

class SpawnPoint {
    static defaultIid { DEFAULT_IID }
    static defaultLevel { DEFAULT_LEVEL }

    // The current spawn: [iid, level identifier].
    static current {
        if (FlagManager.hasStringFlag(IID_FLAG) && FlagManager.hasStringFlag(LEVEL_FLAG)) {
            var iid = FlagManager.getStringFlag(IID_FLAG)
            var level = FlagManager.getStringFlag(LEVEL_FLAG)
            if (iid != null && iid != "" && level != null && level != "") {
                return [iid, level]
            }
        }
        return [DEFAULT_IID, DEFAULT_LEVEL]
    }

    // Makes the Spawn with this iid (in the level called `level`) the current one, and saves it.
    static set(iid, level) {
        if (iid == null || iid == "" || level == null || level == "") {
            return
        }
        FlagManager.setStringFlag(IID_FLAG, iid)
        FlagManager.setStringFlag(LEVEL_FLAG, level)
        FlagManager.save("flags.sav")
    }
}
