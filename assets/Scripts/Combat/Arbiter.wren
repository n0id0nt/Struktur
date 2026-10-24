// Combat/Arbiter.wren
// The one place a contest between two combatants is decided - every roll and stat comparison in a fight
// routes through here, so tuning the whole battle model means editing this file (and the attribute blocks
// in Combat/Stats.wren / Combat/Config/Archetype.wren). Combat/Disruption.wren is the stagger formula this
// module will absorb once poise / might feed into it.
//
// Decided here so far:
//   parry - how well a parry attempt worked: perfect / partial / fail (ParryResult below)
// Planned (the hook points they belong to): hit/evade + damage (might vs guard, crits) in
// CombatResolver.finishResolve, stagger (might vs poise) in Disruption.delay, charge rate (agility) in
// Timeline.tick, stamina regen (endurance) in CombatSession.regenStamina, heal strength (focus) in
// Combatant.use, flee chance (agility) in CombatResolver.flee.
//
// --- Parry -------------------------------------------------------------------------------------------
//   score = timing + clamp((defender reflex - attacker precision) x PARRY_STAT_WEIGHT, +-PARRY_STAT_EDGE_MAX)
//           + roll(+-PARRY_ROLL_SPREAD/2) - (defender exhausted ? PARRY_EXHAUSTED_PENALTY : 0)
//     timing - 0..1, how far through the parry window the defender committed (0 = the instant the prompt
//              opened, 1 = at impact). Waiting longer reads the attack better - but if the window runs out
//              there is no parry at all (ParryPromptState). AI defenders draw it from AI_TIMING_MIN..MAX.
//   score >= PARRY_PERFECT_SCORE -> perfect: no damage, attacker stalled, defender keeps its own move
//   score >= PARRY_PARTIAL_SCORE -> partial: PARTIAL_DAMAGE_THROUGH of the damage, defender's move lost
//   otherwise                    -> fail:    full damage, defender's move lost (it dropped it to guard)
// The roll happens when the defender commits (so the parry pose plays at once) and is applied at impact
// (CombatResolver.finishResolve).
import "math" for Math
import "random" for Random

var PARRY_STAT_WEIGHT       = 0.03   // score per point of reflex over the attacker's precision
var PARRY_STAT_EDGE_MAX     = 0.3    // cap on the stat edge either way, so timing always matters
var PARRY_ROLL_SPREAD       = 0.2    // total width of the random nudge (+-0.1)
var PARRY_EXHAUSTED_PENALTY = 0.15   // a defender below Stats' exhaustion threshold parries worse
var PARRY_PERFECT_SCORE     = 0.8
var PARRY_PARTIAL_SCORE     = 0.45
var PARTIAL_DAMAGE_THROUGH  = 0.5    // share of the attack's damage a partial parry lets through
var AI_TIMING_MIN           = 0.2    // AI defenders have no prompt to time - they draw a timing in this range
var AI_TIMING_MAX           = 0.9

// Set true to print every parry roll's breakdown to the console while tuning.
var LOG_ROLLS = true

var RNG = Random.new()

// The outcome of one parry attempt - stored on the attacker's Timeline entry until the attack lands.
class ParryResult {
    construct new(tier, damageThrough, stallsAttacker, defenderKeepsMove, score) {
        _tier = tier
        _damageThrough = damageThrough
        _stallsAttacker = stallsAttacker
        _defenderKeepsMove = defenderKeepsMove
        _score = score
    }

    // "perfect" / "partial" / "fail"
    tier { _tier }
    // 0..1 share of the attack's damage that still lands (Combatant.use takes 1 - this as cancelFraction).
    damageThrough { _damageThrough }
    // True when the attacker sits out a Combat/Config/Moves.stalled beat afterwards.
    stallsAttacker { _stallsAttacker }
    // True when the defender's own in-flight move survives the parry (otherwise it is cleared, no stagger).
    defenderKeepsMove { _defenderKeepsMove }
    // True when the parry turned at least some of the blow aside (sparks rather than a clean hit).
    deflected { _damageThrough < 1 }
    score { _score }

    static perfect(score) { ParryResult.new("perfect", 0, true, true, score) }
    static partial(score) { ParryResult.new("partial", PARTIAL_DAMAGE_THROUGH, false, false, score) }
    static fail(score) { ParryResult.new("fail", 1, false, false, score) }
}

class Arbiter {
    // defender / attacker: Combatants. timing: 0..1 through the parry window (see header). Returns a ParryResult.
    static parry(defender, attacker, timing) {
        var edge = Math.clamp((defender.stats.reflex - attacker.stats.precision) * PARRY_STAT_WEIGHT,
                              -PARRY_STAT_EDGE_MAX, PARRY_STAT_EDGE_MAX)
        var roll = (RNG.float() - 0.5) * PARRY_ROLL_SPREAD
        var penalty = defender.stats.exhausted ? PARRY_EXHAUSTED_PENALTY : 0
        var score = Math.clamp(timing, 0, 1) + edge + roll - penalty

        var result = null
        if (score >= PARRY_PERFECT_SCORE) {
            result = ParryResult.perfect(score)
        } else if (score >= PARRY_PARTIAL_SCORE) {
            result = ParryResult.partial(score)
        } else {
            result = ParryResult.fail(score)
        }
        if (LOG_ROLLS) {
            System.print("[Arbiter] parry %(defender.name) vs %(attacker.name): timing %(Arbiter.fmt_(timing)) edge %(Arbiter.fmt_(edge)) roll %(Arbiter.fmt_(roll)) exhausted -%(Arbiter.fmt_(penalty)) = %(Arbiter.fmt_(score)) -> %(result.tier)")
        }
        return result
    }

    // An AI defender's stand-in for the player's press timing.
    static aiParryTiming() {
        return AI_TIMING_MIN + RNG.float() * (AI_TIMING_MAX - AI_TIMING_MIN)
    }

    // Two-decimal string for the roll log.
    static fmt_(n) { ((n * 100).round / 100).toString }
}
