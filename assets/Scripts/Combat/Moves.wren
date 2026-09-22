// Combat/Moves.wren
// The move catalog - Phase 5 slices it into per-archetype kits (see Combat/Archetype.wren). Each
// getter mints a fresh Move (immutable value objects), so a caller builds its list once and reuses
// those instances.
//
// Move.new(name, timeCost, damage, baseDelay, staminaCost, staminaRestore, healAmount, curveId,
//          particleId)
//
// The triangle these kits are tuned to hold (roadmap "expect churn" - these numbers will move):
//   Speed  > Power    - 2u moves interrupt-lock a Power wind-up, and stay cheap enough to sustain it
//   Power  > Control   - one Crush/Onslaught that lands is >half a mage's HP; Control burns stamina
//                        holding the lock and gets caught exhausted
//   Control > Speed    - Slow wrecks Speed's rhythm and Mend out-heals its chip damage
import "Combat/Move" for Move

class Moves {
    // --- Speed / Rogue: fast, cheap, glassy -------------------------------------
    static slash  { Move.new("Slash", 2, 15, 2, 9, 0, 0, "quickSlash", "slashSpark") }
    static flurry { Move.new("Flurry", 3, 24, 2, 15, 0, 0, "flurryJab", "flurrySparks") }
    static dodge  { Move.new("Dodge", 2, 0, 0, 0, 22, 0, null, null) }   // slip back, catch breath

    // --- Power / Warrior: slow, devastating, stamina-hungry -------------------
    static cleave    { Move.new("Cleave", 4, 36, 3, 22, 0, 0, "heavySwing", "cleaveBurst") }
    static crush     { Move.new("Crush", 6, 68, 5, 38, 0, 0, "heavySlam", "crushShock") }
    static onslaught { Move.new("Onslaught", 8, 105, 6, 52, 0, 0, "onslaughtCharge", "onslaughtBlast") }

    // --- Control / Mage: mid, tricky, disruptive -----------------------------
    static bolt { Move.new("Bolt", 3, 28, 2, 16, 0, 0, "boltDart", "boltSpark") }
    static slow { Move.new("Slow", 3, 8, 7, 22, 0, 0, "slowPulse", "slowMist") }      // tiny hit, huge stagger
    static mend { Move.new("Mend", 4, 0, 0, 24, 0, 42, null, null) }     // heal self

    static speedKit   { [Moves.slash, Moves.flurry, Moves.dodge] }
    static powerKit    { [Moves.cleave, Moves.crush, Moves.onslaught] }
    static controlKit  { [Moves.bolt, Moves.slow, Moves.mend] }
}
