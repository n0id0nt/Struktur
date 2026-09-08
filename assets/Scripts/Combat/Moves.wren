// Combat/Moves.wren
// The move catalog - Phase 4 of the roadmap ("move definitions become data, not one-off methods").
// One flat table for now; Phase 5 slices it into per-archetype kits, Phase 6 has combos reference
// entries here. Each getter mints a fresh Move (they're immutable value objects), so a caller builds
// its combatant's list once and reuses those instances.
//
// Move.new(name, timeCost, damage, baseDelay, staminaCost, staminaRestore)
import "Combat/Move" for Move

class Moves {
    // --- Player kit -----------------------------------------------------------
    static jab       { Move.new("Jab", 2, 20, 2, 14, 0) }        // Tier 1: cheap, fast, reliable interrupt
    static strike    { Move.new("Strike", 4, 42, 3, 26, 0) }     // Tier 2: bread and butter
    static heavyBlow { Move.new("Heavy Blow", 6, 55, 5, 40, 0) } // Tier 3: big hit + big stagger, big cost
    static guard     { Move.new("Guard", 3, 0, 0, 0, 30) }       // recover stamina; never interrupts

    static playerKit { [Moves.jab, Moves.strike, Moves.heavyBlow, Moves.guard] }

    // --- Enemy moves -------------------------------------------------------
    // A critter's single attack, built from its own combat* getters (see GameObjects/Critter.wren).
    static critterAttack(script) {
        return Move.new("Attack", script.combatMoveCost, script.combatAttack, script.combatBaseDelay,
                        script.combatStaminaCost, 0)
    }
}
