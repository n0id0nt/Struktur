// World/StoryGates.wren
// Shows a level-placed entity only during part of the story. An NPC placed in LDtk can carry:
//   Versions (Array<Int>) - the "arc1_stage" values it is shown in, e.g. [9, 10] (empty = always)
// and, for older entities:
//   ShowFromStage (Int)  - hidden until the "arc1_stage" flag reaches this (empty = from the start)
//   HideFromStage (Int)  - hidden once "arc1_stage" reaches this (empty = never)
//   HideIfFlag (String)  - hidden while this int flag is non-zero (e.g. "healed_wolf")
// and, set from a script rather than LDtk (withFlag - e.g. GameObjects/NPC.wren FLAG_GATES), an int flag that has to
// hold one value - Thomas is in his cabin, at the training grounds or at the north gate by "thomas_place".
// so one level can hold Finn in the workshop at the start and again in the evening, a guardian that steps aside
// after its fight, and so on. Scripts call register() from start(); the gameplay state (States/GameWorldState.wren)
// calls refresh() every update, which setActive/setInactives each entity when its visibility changes (an inactive
// entity draws nothing, has no physics body and runs no script). isVisible() lets the interact scan skip hidden ones.
import "flags" for FlagManager
import "gameObject" for GameObject

// The int flag the story's progress lives in (Dialogue/Arc1/ sets it).
var STAGE_FLAG = "arc1_stage"

class StoryGates {
    static stage { FlagManager.getIntFlag(STAGE_FLAG) }

    // A gate from an entity's script args, or null if it has no gate fields set (always visible).
    static fromArgs(args) {
        var showFrom = StoryGates.number_(args["ShowFromStage"])
        var hideFrom = StoryGates.number_(args["HideFromStage"])
        var hideIfFlag = args["HideIfFlag"]
        if (hideIfFlag == "") {
            hideIfFlag = null
        }
        // Arrays reach scripts as one comma-separated string ("9,10").
        var versions = null
        var versionsText = args["Versions"]
        if (versionsText is String && versionsText != "") {
            versions = versionsText.split(",").map {|version| Num.fromString(version.trim()) }.where {|version| version != null }.toList
            if (versions.count == 0) {
                versions = null
            }
        }
        if (showFrom == null && hideFrom == null && hideIfFlag == null && versions == null) {
            return null
        }
        return [showFrom, hideFrom, hideIfFlag, versions]
    }

    // `gate` (null for none yet) also needing int flag `flag` to equal `value` - the gate to register.
    static withFlag(gate, flag, value) {
        var result = gate == null ? [null, null, null, null] : gate
        result.add([flag, value])
        return result
    }

    static register(entity, gate) {
        if (gate == null) {
            return
        }
        if (__entries == null) {
            __entries = {}
        }
        // [gate, currently visible] - visibility starts unknown (null) so the first refresh always applies it.
        __entries[entity] = [gate, null]
        StoryGates.apply_(entity)
    }

    static unregister(entity) {
        if (__entries != null) {
            __entries.remove(entity)
        }
    }

    static isVisible(entity) {
        if (__entries == null || !__entries.containsKey(entity)) {
            return true
        }
        return StoryGates.evaluate_(__entries[entity][0])
    }

    static refresh() {
        if (__entries == null) {
            return
        }
        var gone = []
        for (entity in __entries.keys) {
            if (!GameObject.isValid(entity)) {
                gone.add(entity)
            } else {
                StoryGates.apply_(entity)
            }
        }
        for (entity in gone) {
            __entries.remove(entity)
        }
    }

    static clear() {
        __entries = null
    }

    static apply_(entity) {
        var entry = __entries[entity]
        var visible = StoryGates.evaluate_(entry[0])
        if (visible == entry[1]) {
            return
        }
        entry[1] = visible
        if (visible) {
            GameObject.setActive(entity)
        } else {
            GameObject.setInactive(entity)
        }
    }

    static evaluate_(gate) {
        var stage = StoryGates.stage
        if (gate[0] != null && stage < gate[0]) {
            return false
        }
        if (gate[1] != null && stage >= gate[1]) {
            return false
        }
        if (gate[2] != null && FlagManager.getIntFlag(gate[2]) != 0) {
            return false
        }
        if (gate[3] != null && !gate[3].contains(stage)) {
            return false
        }
        if (gate.count > 4 && FlagManager.getIntFlag(gate[4][0]) != gate[4][1]) {
            return false
        }
        return true
    }

    // LDtk Int fields arrive as numbers; an empty optional field as "" or null.
    static number_(value) {
        if (value == null || value == "") {
            return null
        }
        if (value is Num) {
            return value
        }
        return Num.fromString(value)
    }
}
