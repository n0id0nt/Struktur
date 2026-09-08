// Combat/Stats.wren
// The numeric core of a battle participant: HP and stamina (Phase 4 of the Interrupt Combat
// Roadmap). Later phases hang resistances and status timers off this same bag, so systems that only
// care about "how hurt / how tired is this thing" never need to know about Combatant, moves, or the
// timeline.
var EXHAUSTION_THRESHOLD = 0.2   // below this fraction of max stamina, moves take +1 time unit

class Stats {
    construct new(maxHp, maxStamina) {
        _maxHp = maxHp
        _hp = maxHp
        _maxStamina = maxStamina
        _stamina = maxStamina
    }

    hp { _hp }
    maxHp { _maxHp }
    alive { _hp > 0 }
    fraction { _maxHp > 0 ? _hp / _maxHp : 0 }

    stamina { _stamina }
    maxStamina { _maxStamina }
    staminaFraction { _maxStamina > 0 ? _stamina / _maxStamina : 0 }
    // The Disruption/Timeline exhaustion penalty gate (see Timeline.commit).
    exhausted { staminaFraction < EXHAUSTION_THRESHOLD }

    takeDamage(amount) {
        _hp = _hp - amount
        if (_hp < 0) {
            _hp = 0
        }
    }

    heal(amount) {
        _hp = _hp + amount
        if (_hp > _maxHp) {
            _hp = _maxHp
        }
    }

    spendStamina(amount) {
        _stamina = _stamina - amount
        if (_stamina < 0) {
            _stamina = 0
        }
    }

    // Continuous regen (Timeline.tick feeds this) and Guard's one-shot restore both land here.
    gainStamina(amount) {
        _stamina = _stamina + amount
        if (_stamina > _maxStamina) {
            _stamina = _maxStamina
        }
    }
}
