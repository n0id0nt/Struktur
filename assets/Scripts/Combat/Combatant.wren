// Combat/Combatant.wren
// One participant in a battle - the "Critter" of the combat system. Wraps a Stats bag (HP), a
// display name, the list of Moves it can commit to on the Timeline (Phase 2 - see Combat/Move.wren
// and Combat/Timeline.wren), and the overworld entity it came from, so a defeated enemy can be
// removed from the field.
import "Combat/Stats" for Stats

class Combatant {
    // entity may be null for a purely synthetic combatant. moves is a non-empty list of Move.
    construct new(name, maxHp, moves, entity) {
        _name = name
        _stats = Stats.new(maxHp)
        _moves = moves
        _entity = entity
    }

    name { _name }
    stats { _stats }
    moves { _moves }
    entity { _entity }
    alive { _stats.alive }

    // Apply a move's flat damage to `target`. Returns the amount dealt for the battle log. Phase 2:
    // no timing bonus, no mitigation - the charge already happened on the Timeline.
    use(move, target) {
        target.stats.takeDamage(move.damage)
        return move.damage
    }
}
