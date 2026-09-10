// Combat/Move.wren
// A single combat action: a name, a charge cost in time units (see Combat/Timeline.wren), flat
// damage, a base interrupt delay (Phase 3 - Combat/Disruption.wren), a stamina cost, a stamina
// restore, and an HP heal-self amount (Phase 4-5). A move fires the instant its bar fills; later
// phases add element and status effects to this same object.
class Move {
    // baseDelay: 2-6 time units (0 for non-offensive moves - they never interrupt).
    // staminaCost / staminaRestore / healAmount: paid / gained / healed when the move resolves.
    construct new(name, timeCost, damage, baseDelay, staminaCost, staminaRestore, healAmount) {
        _name = name
        _timeCost = timeCost
        _damage = damage
        _baseDelay = baseDelay
        _staminaCost = staminaCost
        _staminaRestore = staminaRestore
        _healAmount = healAmount
    }

    name { _name }
    timeCost { _timeCost }
    damage { _damage }
    baseDelay { _baseDelay }
    staminaCost { _staminaCost }
    staminaRestore { _staminaRestore }
    healAmount { _healAmount }

    // True for anything that picks an enemy target (vs. a self-buff / heal).
    offensive { _damage > 0 }

    // "Strike  4u  -26sp" / "Mend  4u  +42hp" / "Dodge  2u  +22sp" - for the move-menu buttons.
    menuLabel {
        if (_healAmount > 0) {
            return "%(_name)  %(_timeCost)u  +%(_healAmount)hp"
        }
        if (_staminaRestore > 0) {
            return "%(_name)  %(_timeCost)u  +%(_staminaRestore)sp"
        }
        return "%(_name)  %(_timeCost)u  -%(_staminaCost)sp"
    }
}
