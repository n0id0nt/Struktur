// Combat/Stats.wren
// The numeric core of a battle participant, per the animation-timed battle system doc:
//   health / stamina pools      attack   scales physical damage: x (1 + Attack / 100)
//   mana     scales spell damage and healing the same way      defence  passive mitigation, added to armour defence
//   recovery stamina per second while in Hold - the only time stamina comes back
// There is no Speed stat: nothing can scale time. HP persists between fights for the party (the Combatant is kept);
// stamina is topped up when a fight ends.
var ATTRIBUTE_NAMES = ["attack", "mana", "defence", "recovery"]

class Stats {
    construct new(maxHp, maxStamina, attack, mana, defence, recovery) {
        _maxHp = maxHp
        _hp = maxHp
        _maxStamina = maxStamina
        _stamina = maxStamina
        _attack = attack
        _mana = mana
        _defence = defence
        _recovery = recovery
    }

    // The early / mid / late combatant tiers of the battle system doc.
    static tier(tier) {
        if (tier >= 2) return Stats.new(280, 140, 45, 45, 30, 20)
        if (tier == 1) return Stats.new(180, 110, 25, 25, 15, 16)
        return Stats.new(100, 80, 10, 10, 5, 12)
    }

    attack { _attack }
    mana { _mana }
    defence { _defence }
    recovery { _recovery }
    attack=(v) { _attack = v }
    mana=(v) { _mana = v }
    defence=(v) { _defence = v }
    recovery=(v) { _recovery = v }

    // By name - the inventory's stat rows (ATTRIBUTE_NAMES).
    attr(name) {
        if (name == "attack") return _attack
        if (name == "mana") return _mana
        if (name == "defence") return _defence
        return _recovery
    }

    hp { _hp }
    maxHp { _maxHp }
    alive { _hp > 0 }
    fraction { _maxHp > 0 ? _hp / _maxHp : 0 }

    stamina { _stamina }
    maxStamina { _maxStamina }
    staminaFraction { _maxStamina > 0 ? _stamina / _maxStamina : 0 }

    setMaxHp(value) {
        _maxHp = value
        _hp = value
    }
    setMaxStamina(value) {
        _maxStamina = value
        _stamina = value
    }

    takeDamage(amount) {
        _hp = _hp - amount
        if (_hp < 0) _hp = 0
    }

    // Returns how much of `amount` was over the maximum (the overheal).
    heal(amount) {
        _hp = _hp + amount
        var over = _hp - _maxHp
        if (over > 0) {
            _hp = _maxHp
            return over
        }
        return 0
    }

    // Brings a saved value back (party.sav).
    setHp(value) {
        _hp = value < 0 ? 0 : (value > _maxHp ? _maxHp : value)
    }

    spendStamina(amount) {
        _stamina = _stamina - amount
        if (_stamina < 0) _stamina = 0
    }

    // Returns how much of `amount` was over the maximum.
    gainStamina(amount) {
        _stamina = _stamina + amount
        var over = _stamina - _maxStamina
        if (over > 0) {
            _stamina = _maxStamina
            return over
        }
        return 0
    }
}
