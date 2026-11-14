// World/StoryGates.wren
// Shows a level-placed entity only during part of the story. An NPC placed in LDtk can carry:
//   Versions (Array<Int>) - the "arc1_stage" values it is shown in, e.g. [9, 10] (empty = always)
// and, for older entities:
//   ShowFromStage (Int)  - hidden until the "arc1_stage" flag reaches this (empty = from the start)
//   HideFromStage (Int)  - hidden once "arc1_stage" reaches this (empty = never)
//   HideIfFlag (String)  - hidden while this int flag is non-zero (e.g. "healed_wolf")
// so one level can hold Finn in the workshop at the start and again in the evening, a guardian that steps aside
// after its fight, and so on. Scripts call register() from start(); the gameplay state (States/GameWorldState.wren)
// calls refresh() every update, which setActive/setInactives each entity when its visibility changes (an inactive
// entity draws nothing, has no physics body and runs no script). isVisible() lets the interact scan skip hidden ones.
import "flags" for FlagManager
import "gameObject" for GameObject

// The int flag the story's progress lives in (Dialogue/Arc1Dialogue.json sets it).
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
            versions = versionsText.split(",").map {|v| Num.fromString(v.trim()) }.where {|v| v != null }.toList
            if (versions.count == 0) {
                versions = null
            }
        }
        if (showFrom == null && hideFrom == null && hideIfFlag == null && versions == null) {
            return null
        }
        return [showFrom, hideFrom, hideIfFlag, versions]
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
