// PressurePlates.wren
// Who is standing on a pressure plate right now, across every plate (GameObjects/PressurePlate.wren reports
// steps on and off). Kept in a plain module rather than as statics on the plate script, since registered
// game-object scripts run in their own module instance - this is the copy everyone shares. Party members use it
// to offer to hold a plate down (GameObjects/PartyMember.wren). The gameplay state clears it on exit.
class PressurePlates {
    // True while entity is standing on any plate.
    static isStoodOn(entity) { __standing != null && __standing.containsKey(entity) }

    // delta: +1 stepping onto a plate, -1 stepping off. Counted per entity, so standing across two plates at once
    // stays "on a plate" until it leaves both.
    static track(entity, delta) {
        if (__standing == null) {
            __standing = {}
        }
        var count = (__standing[entity] == null ? 0 : __standing[entity]) + delta
        if (count > 0) {
            __standing[entity] = count
        } else {
            __standing.remove(entity)
        }
    }

    static clear() {
        __standing = null
    }
}
