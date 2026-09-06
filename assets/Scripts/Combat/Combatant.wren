// Combat/Combatant.wren
// One participant in a battle - the "Critter" of the combat system. Wraps a Stats bag (HP) plus the
// bits a fight needs to name and resolve it: a display name, a flat attack value (Phase 1 has no
// move list yet - everyone just "Attacks"), and the overworld entity it came from, so a defeated
// enemy can be removed from the field (and, later, so attack VFX can be aimed at real sprites).
import "Combat/Stats" for Stats

class Combatant {
    // entity may be null for a purely synthetic combatant.
    construct new(name, maxHp, attackPower, entity) {
        _name = name
        _stats = Stats.new(maxHp)
        _attackPower = attackPower
        _entity = entity
    }

    name { _name }
    stats { _stats }
    attackPower { _attackPower }
    entity { _entity }
    alive { _stats.alive }

    // Phase 1: flat damage, no timing, no mitigation. Returns the amount actually dealt so the
    // battle log can report it.
    attack(target) {
        target.stats.takeDamage(_attackPower)
        return _attackPower
    }
}
