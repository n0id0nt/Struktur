// Combat/Move.wren
// A single combat action. Phase 2 of the Interrupt Combat Roadmap: a name, a charge cost in time
// units (see Combat/Timeline.wren for what a unit is worth), and flat damage. There's no cast time
// separate from the charge yet - a move fires the instant its bar fills. Later phases add stamina
// cost, element, and effects to this same object.
class Move {
    construct new(name, timeCost, damage) {
        _name = name
        _timeCost = timeCost
        _damage = damage
    }

    name { _name }
    timeCost { _timeCost }
    damage { _damage }

    // "Strike  4u" - for the move-menu buttons (damage shows in the battle log when it lands).
    menuLabel { "%(_name)  %(_timeCost)u" }
}
