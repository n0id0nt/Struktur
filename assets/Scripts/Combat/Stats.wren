// Combat/Stats.wren
// The numeric core of a battle participant - just HP for now (Phase 0/1 of the Interrupt Combat
// Roadmap). Later phases hang stamina, resistances and status timers off this same bag, so systems
// that only care about "how hurt is this thing" never need to know about Combatant, moves, or the
// timeline.
class Stats {
    construct new(maxHp) {
        _maxHp = maxHp
        _hp = maxHp
    }

    hp { _hp }
    maxHp { _maxHp }
    alive { _hp > 0 }
    // 0..1 - for health bars and threshold checks.
    fraction { _maxHp > 0 ? _hp / _maxHp : 0 }

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
}
