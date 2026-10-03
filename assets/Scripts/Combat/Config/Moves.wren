// Combat/Config/Moves.wren
// The move catalog - Phase 5 slices it into per-archetype kits (see Combat/Config/Archetype.wren). Each
// getter mints a fresh Move (immutable value objects), so a caller builds its list once and reuses
// those instances.
//
// Move.new(name, timeCost, damage, baseDelay, staminaCost, staminaRestore, healAmount, curveId,
//          particleId)
// Move.new(..., particleId, parryPromptTime) - offensive moves only; game seconds BEFORE the attack lands at
//          which the target gets the parry prompt (States/Combat/ParryPromptState.wren, time slowed while it is
//          up). Capped to 80% of the attack's length (CombatSession.nextParryOffer). Placeholder numbers.
//
// The triangle these kits are tuned to hold (roadmap "expect churn" - these numbers will move):
//   Speed  > Power    - 2u moves interrupt-lock a Power wind-up, and stay cheap enough to sustain it
//   Power  > Control   - one Crush/Onslaught that lands is >half a mage's HP; Control burns stamina
//                        holding the lock and gets caught exhausted
//   Control > Speed    - Slow wrecks Speed's rhythm and Mend out-heals its chip damage
import "Combat/Move" for Move

// Time units an attacker sits doing nothing after a parry actually lands on their attack (Combat/
// Config/Moves.stalled, committed by States/Combat/CombatResolver.wren instead of letting them pick again
// immediately) - ~1.2s of real time via Combat/Timeline.SECONDS_PER_TIME_UNIT.
var STALL_TIME_COST = 3

class Moves {
    // --- Speed / Rogue: fast, cheap, glassy -------------------------------------
    static slash  { Move.new("Slash", 2, 15, 2, 9, 0, 0, "quickSlash", "slashSpark", 0.35) }
    static flurry { Move.new("Flurry", 3, 24, 2, 15, 0, 0, "flurryJab", "flurrySparks", 0.4) }
    static dodge  { Move.new("Dodge", 2, 0, 0, 0, 22, 0, null, null) }   // slip back, catch breath

    // --- Power / Warrior: slow, devastating, stamina-hungry -------------------
    static cleave    { Move.new("Cleave", 4, 36, 3, 22, 0, 0, "heavySwing", "cleaveBurst", 0.5) }
    static crush     { Move.new("Crush", 6, 68, 5, 38, 0, 0, "heavySlam", "crushShock", 0.7) }
    static onslaught { Move.new("Onslaught", 8, 105, 6, 52, 0, 0, "onslaughtCharge", "onslaughtBlast", 0.9) }

    // --- Control / Mage: mid, tricky, disruptive -----------------------------
    static bolt { Move.new("Bolt", 3, 28, 2, 16, 0, 0, "boltDart", "boltSpark", 0.45) }
    static slow { Move.new("Slow", 3, 8, 7, 22, 0, 0, "slowPulse", "slowMist", 0.45) }      // tiny hit, huge stagger
    static mend { Move.new("Mend", 4, 0, 0, 24, 0, 42, null, null) }     // heal self

    // A do-nothing move: forced onto an attacker's timeline slot after their target chooses to parry
    // (States/Combat/ParryPromptState.wren), so they sit out a beat instead of acting again
    // immediately. Never offensive, never parryable itself.
    static stalled { Move.new("Stalled", STALL_TIME_COST, 0, 0, 0, 0, 0, null, null) }

    static speedKit   { [Moves.slash, Moves.flurry, Moves.dodge] }
    static powerKit    { [Moves.cleave, Moves.crush, Moves.onslaught] }
    static controlKit  { [Moves.bolt, Moves.slow, Moves.mend] }
}
