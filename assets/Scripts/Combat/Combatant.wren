// Combat/Combatant.wren
// One participant in a battle. Wraps a Stats bag (HP + stamina), a display name, the move kit it
// commits to on the Timeline, and the overworld entity it came from (null for the player).
import "Combat/Stats" for Stats

class Combatant {
    construct new(name, maxHp, maxStamina, moves, entity) {
        _name = name
        _stats = Stats.new(maxHp, maxStamina)
        _moves = moves
        _entity = entity
    }

    name { _name }
    stats { _stats }
    moves { _moves }
    entity { _entity }
    alive { _stats.alive }

    // Resolve a move: damage the target, pay/gain stamina, heal self. Returns damage dealt for the
    // battle log. No timing bonus or mitigation here - the charge already happened on the Timeline,
    // and the interrupt (Combat/Disruption.wren) is handled by CombatState.
    use(move, target) {
        if (move.damage > 0) {
            target.stats.takeDamage(move.damage)
        }
        _stats.spendStamina(move.staminaCost)
        _stats.gainStamina(move.staminaRestore)
        if (move.healAmount > 0) {
            _stats.heal(move.healAmount)
        }
        return move.damage
    }
}
