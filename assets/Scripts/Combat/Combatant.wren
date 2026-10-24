// Combat/Combatant.wren
// One participant in a battle. Wraps a Stats bag (HP + stamina + attributes), a display name, the move kit
// it commits to on the Timeline, and the overworld entity it came from (null for the player).
import "Combat/Stats" for Stats

class Combatant {
    // attributes: the full attribute block (see Combat/Stats.wren) - defs build it with Archetype.attributesWith.
    construct new(name, maxHp, maxStamina, attributes, moves, entity) {
        _name = name
        _stats = Stats.new(maxHp, maxStamina, attributes)
        _moves = moves
        _entity = entity
    }

    name { _name }
    stats { _stats }
    moves { _moves }
    entity { _entity }
    alive { _stats.alive }

    // Resolve a move: damage the target, pay/gain stamina, heal self. Returns damage dealt for the
    // battle log. cancelFraction is 0..1, the share of the damage the target's parry blocked (1 - the tier's
    // damageThrough - see Combat/Arbiter.wren ParryResult), 0 with no parry (including anything not parryable).
    // The charge itself already happened on the Timeline, and the interrupt (Combat/Disruption.wren)
    // is handled by States/Combat/CombatResolver.wren.
    use(move, target, cancelFraction) {
        var dealt = 0
        if (move.damage > 0) {
            dealt = (move.damage * (1 - cancelFraction)).round
            target.stats.takeDamage(dealt)
        }
        _stats.spendStamina(move.staminaCost)
        _stats.gainStamina(move.staminaRestore)
        if (move.healAmount > 0) {
            _stats.heal(move.healAmount)
        }
        return dealt
    }
}
