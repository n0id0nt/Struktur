// Combat/Combatant.wren
// One participant in a battle - the "Critter" of the combat system. Wraps a Stats bag (HP +
// stamina), a display name, the list of Moves it can commit to on the Timeline, and the overworld
// entity it came from, so a defeated enemy can be removed from the field.
import "Combat/Stats" for Stats

class Combatant {
    // entity may be null for a purely synthetic combatant. moves is a non-empty list of Move.
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

    // Resolve a move: damage the target, then pay/gain the move's stamina. Returns damage dealt for
    // the battle log. No timing bonus or mitigation here - the charge already happened on the
    // Timeline, and the interrupt (Combat/Disruption.wren) is handled by CombatState.
    use(move, target) {
        target.stats.takeDamage(move.damage)
        _stats.spendStamina(move.staminaCost)
        _stats.gainStamina(move.staminaRestore)
        return move.damage
    }
}
