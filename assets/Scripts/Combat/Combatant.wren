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

    // Apply a resolved move: damage the target, pay/gain stamina, heal self. `dealt` and `healed` are already
    // arbitrated (Combat/Arbiter.hit / heal, scaled by any parry - see States/Combat/CombatResolver.wren, which
    // also handles the charge's stagger); this only applies them.
    use(move, target, dealt, healed) {
        if (dealt > 0) {
            target.stats.takeDamage(dealt)
        }
        _stats.spendStamina(move.staminaCost)
        _stats.gainStamina(move.staminaRestore)
        if (healed > 0) {
            _stats.heal(healed)
        }
    }
}
