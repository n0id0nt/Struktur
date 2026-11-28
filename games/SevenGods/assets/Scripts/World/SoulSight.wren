// World/SoulSight.wren
// Soul Sight - the Guide's gift from the shrine's Heart (Scene 9). Once the player has it (the ability_soul_sight
// flag, set by the fragment scene), the SoulSight input switches it on and off, out in the world (States/
// GameWorldState.wren). While it's on:
//   - the screen takes a cold blue tint and the player gives off soul light (UI/Gameplay/SoulSightOverlay.wren,
//     Util/SoulFx.wren);
//   - hidden doors (GameObjects/Door.wren, DoorType "Hidden") glow and can be opened, and hidden plates
//     (GameObjects/PressurePlate.wren, "Hidden") show and can be pressed.
// Every change sends the soulSight game event (Events/GameEvents.wren); everything above reacts to that event - nothing
// watches this state. It isn't saved: a new session starts with it off.
//
// In a fight it's a separate thing (States/CombatState.wren): always on once the player has it, whatever the world's
// switch says. Its strength grows with every Soul Fragment carried (level): `reach`
// is how many seconds ahead it lets the timeline show the foes' coming moves (UI/Combat/TimelineUI.wren) - any move
// that starts inside it, however long it runs.
import "flags" for FlagManager
import "Events/GameEvents" for GameEvents
import "Inventory/Inventory" for Inventory

var ABILITY_FLAG = "ability_soul_sight"
var FRAGMENT_ITEM = "Soul Fragment"
// Seconds of the foes' future a fight's Soul Sight shows, by level (Soul Fragments carried): the first fragment's
// glimpse, widening with each one after. Past the end of the list, the last entry.
var REACH = [0.2, 0.5, 0.9, 1.4, 2.0, 2.7, 3.5]

class SoulSight {
    // True once the fragment scene has given it.
    static available { FlagManager.getIntFlag(ABILITY_FLAG) >= 1 }

    static active { __active == true }

    // Its strength: one level per Soul Fragment carried (0 before the first) - or the battle test's pick.
    static level { __levelOverride != null ? __levelOverride : Inventory.count(FRAGMENT_ITEM) }

    // The highest level with a reach of its own.
    static maxLevel { REACH.count }

    // A level forced from outside the save - the battle test's Soul Sight picker (States/BattleTestState.wren) - or
    // null to go by the Soul Fragments carried. While set, it stands in for having the gift too.
    static levelOverride { __levelOverride }
    static levelOverride=(value) { __levelOverride = value }

    // Seconds ahead a fight's Soul Sight sees at this level (0 without it).
    static reach {
        if (__levelOverride == null && !SoulSight.available) return 0
        return SoulSight.reachAt(SoulSight.level)
    }

    // Seconds ahead Soul Sight sees at `level` (0 at level 0).
    static reachAt(level) {
        if (level <= 0) return 0
        return REACH[(level - 1).min(REACH.count - 1)]
    }

    // The SoulSight input: on <-> off (ignored until the player has it).
    static toggle() {
        if (!SoulSight.available) return
        SoulSight.set(!SoulSight.active)
    }

    static set(on) {
        if (SoulSight.active == on) return
        __active = on
        GameEvents.soulSightChanged(on)
    }

    // The input that switches it (Settings/InputBindings/InputConfig.json).
    static input { "SoulSight" }

    // Off, quietly - a new session, or the world unloading (nothing is listening to an event then).
    static reset() { __active = false }

    // The soulSight event's data -> on or off.
    static fromEvent(data) { data != null && data["active"] == 1 }
}
