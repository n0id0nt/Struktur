// Combat/Move.wren
// A single combat action: a name, a charge cost in time units (see Combat/Timeline.wren for what a
// unit is worth), flat damage, and a base interrupt delay (Phase 3 - see Combat/Disruption.wren).
// There's no cast time separate from the charge - a move fires the instant its bar fills. Later
// phases add stamina cost, element, and effects to this same object.
class Move {
    // baseDelay: 2-6 time units, the move's raw disruptive weight before power/timing scaling.
    construct new(name, timeCost, damage, baseDelay) {
        _name = name
        _timeCost = timeCost
        _damage = damage
        _baseDelay = baseDelay
    }

    name { _name }
    timeCost { _timeCost }
    damage { _damage }
    baseDelay { _baseDelay }

    // "Strike  4u" - for the move-menu buttons (damage shows in the battle log when it lands).
    menuLabel { "%(_name)  %(_timeCost)u" }
}
