// Combat/Move.wren
// A single combat action: a name, a charge cost in time units (see Combat/Timeline.wren), flat
// damage, a base interrupt delay (Phase 3 - Combat/Disruption.wren), a stamina cost spent on use,
// and a stamina restore granted on use (Phase 4 - for Guard-style recovery moves). A move fires the
// instant its bar fills; later phases add element and effects to this same object.
class Move {
    // baseDelay: 2-6 time units (0 for non-offensive moves - they never interrupt).
    // staminaCost / staminaRestore: points spent / gained when the move resolves.
    construct new(name, timeCost, damage, baseDelay, staminaCost, staminaRestore) {
        _name = name
        _timeCost = timeCost
        _damage = damage
        _baseDelay = baseDelay
        _staminaCost = staminaCost
        _staminaRestore = staminaRestore
    }

    name { _name }
    timeCost { _timeCost }
    damage { _damage }
    baseDelay { _baseDelay }
    staminaCost { _staminaCost }
    staminaRestore { _staminaRestore }

    // "Strike  4u  -26sp" / "Guard  3u  +30sp" - for the move-menu buttons.
    menuLabel {
        if (_staminaRestore > 0) {
            return "%(_name)  %(_timeCost)u  +%(_staminaRestore)sp"
        }
        return "%(_name)  %(_timeCost)u  -%(_staminaCost)sp"
    }
}
