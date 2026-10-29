// Combat/Arbiter.wren
// The one place a contest between two combatants is decided - every roll and stat comparison in a fight
// routes through here, so tuning the whole battle model means editing this file (and the attribute blocks
// in Combat/Stats.wren / Combat/Config/Archetype.wren). Combat/Disruption.wren stays the raw stagger
// formula; stagger() here feeds it the damage actually dealt and the might-vs-poise multiplier.
//
// Decided here so far (CombatResolver.finishResolve asks, Combatant.use applies):
//   parry   - how well a parry attempt worked: perfect / partial / fail (ParryResult below)
//   evade   - whether an unparried attack misses outright: defender agility vs attacker precision
//   hit     - an attack's damage before any parry: might vs guard, a variance spread and a crit roll
//             off precision (HitResult below)
//   stagger - how far a landed hit shoves the target's charge back: dealt damage + might vs poise
//   heal    - how much a heal restores: focus
// Tempo (Combat/Timeline.wren, Combat/Stats.wren and CombatSession ask):
//   moveCost       - a move's effective charge in time units: agility + the exhaustion penalty
//   initiative     - head start on a combatant's first charge of the fight: agility
//   staminaRegen   - stamina regained per stretch of charging: endurance
//   exhaustion     - the stamina fraction below which a combatant counts as exhausted: endurance
// Escape (CombatResolver.flee asks):
//   flee           - whether the party gets away: party agility vs the fastest enemy's
// Estimates for decision-making (Combat/EnemyAI.wren - the AI's choices live there, the rules here):
//   expectedHit    - average damage an attack would deal, crits and evasion folded in, no rolls
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
//
// --- Evade -------------------------------------------------------------------------------------------
//   chance = clamp(EVADE_BASE_CHANCE + (defender agility - attacker precision) x EVADE_PER_POINT, 0, EVADE_CHANCE_MAX)
//            + DODGE_EVADE_BONUS while the defender is charging Dodge, x EVADE_EXHAUSTED_MULT when exhausted.
//   Rolled only when the defender didn't try to parry (a parry is the defender standing its ground). An evaded
//   attack deals nothing and staggers nothing.
//
// --- Hit ---------------------------------------------------------------------------------------------
//   amount = max(1, round(move damage x clamp(1 + (might - guard) x DAMAGE_STAT_WEIGHT, DAMAGE_MULT_MIN..MAX)
//                          x variance(1 +-DAMAGE_VARIANCE/2) x (crit ? CRIT_MULTIPLIER : 1)))
//   crit chance = clamp(CRIT_BASE_CHANCE + (precision - 10) x CRIT_CHANCE_PER_POINT, 0, CRIT_CHANCE_MAX)
//   A parry then scales `amount` by its damageThrough.
//
// --- Stagger -----------------------------------------------------------------------------------------
//   Disruption.delay on the damage actually dealt (so a partly-blocked or well-guarded hit staggers less),
//   x clamp(1 + (attacker might - defender poise) x STAGGER_STAT_WEIGHT, STAGGER_MULT_MIN..MAX).
//
// --- Heal --------------------------------------------------------------------------------------------
//   round(move healAmount x clamp(1 + (focus - 10) x HEAL_STAT_WEIGHT, HEAL_MULT_MIN..MAX))
//
// --- Tempo -------------------------------------------------------------------------------------------
//   move cost  = move timeCost x clamp(1 - (agility - 10) x CHARGE_STAT_WEIGHT, CHARGE_MULT_MIN..MAX)
//                + (exhausted ? EXHAUSTION_TIME_PENALTY : 0)            (a Stalled beat is never scaled)
//     Locked in at commit (Timeline.commit), so the parry window, the clip length and the bar all follow
//     the same effective cost.
//   initiative = clamp((agility - 10) x INITIATIVE_PER_POINT + roll(0..INITIATIVE_ROLL), INITIATIVE_MIN, ..)
//                time units, capped at INITIATIVE_MAX_SHARE of the first move's cost - negative = a late start.
//   regen      = units x STAMINA_REGEN x clamp(1 + (endurance - 10) x REGEN_STAT_WEIGHT, REGEN_MULT_MIN..MAX)
//   exhaustion = clamp(EXHAUSTION_THRESHOLD - (endurance - 10) x EXHAUSTION_PER_POINT, EXHAUSTION_MIN..MAX)
//
// --- Flee --------------------------------------------------------------------------------------------
//   chance = clamp(FLEE_BASE_CHANCE + (average living party agility - fastest living enemy agility)
//                  x FLEE_PER_POINT, FLEE_CHANCE_MIN..MAX)
import "math" for Math
import "random" for Random
import "Combat/Disruption" for Disruption

var PARRY_STAT_WEIGHT       = 0.03   // score per point of reflex over the attacker's precision
var PARRY_STAT_EDGE_MAX     = 0.3    // cap on the stat edge either way, so timing always matters
var PARRY_ROLL_SPREAD       = 0.2    // total width of the random nudge (+-0.1)
var PARRY_EXHAUSTED_PENALTY = 0.15   // a defender below Stats' exhaustion threshold parries worse
var PARRY_PERFECT_SCORE     = 0.8
var PARRY_PARTIAL_SCORE     = 0.45
var PARTIAL_DAMAGE_THROUGH  = 0.5    // share of the attack's damage a partial parry lets through
var AI_TIMING_MIN           = 0.2    // AI defenders have no prompt to time - they draw a timing in this range
var AI_TIMING_MAX           = 0.9

var EVADE_BASE_CHANCE       = 0.05   // at even agility vs precision
var EVADE_PER_POINT         = 0.015  // + per point of defender agility over attacker precision
var EVADE_CHANCE_MAX        = 0.25   // from stats alone (Dodge adds on top)
var DODGE_EVADE_BONUS       = 0.3    // while the defender is charging Dodge - 0 turns it off
var EVADE_EXHAUSTED_MULT    = 0.5

var DAMAGE_STAT_WEIGHT      = 0.03   // damage multiplier per point of attacker might over defender guard
var DAMAGE_MULT_MIN         = 0.6
var DAMAGE_MULT_MAX         = 1.5
var DAMAGE_VARIANCE         = 0.15   // total width of the damage spread (+-7.5%)
var CRIT_BASE_CHANCE        = 0.05   // crit chance at precision 10
var CRIT_CHANCE_PER_POINT   = 0.01   // + per point of precision over 10 (- per point under)
var CRIT_CHANCE_MAX         = 0.3
var CRIT_MULTIPLIER         = 1.5

var STAGGER_STAT_WEIGHT     = 0.04   // stagger multiplier per point of attacker might over defender poise
var STAGGER_MULT_MIN        = 0.5
var STAGGER_MULT_MAX        = 1.5

var HEAL_STAT_WEIGHT        = 0.03   // heal multiplier per point of focus over 10
var HEAL_MULT_MIN           = 0.5
var HEAL_MULT_MAX           = 1.5

var CHARGE_STAT_WEIGHT      = 0.02   // move cost shrinks this much per point of agility over 10 (grows under)
var CHARGE_MULT_MIN         = 0.8
var CHARGE_MULT_MAX         = 1.2
var EXHAUSTION_TIME_PENALTY = 1      // extra time units on a move committed while exhausted

var INITIATIVE_PER_POINT    = 0.1    // time units of head start per point of agility over 10
var INITIATIVE_ROLL         = 0.4    // plus a random 0..this
var INITIATIVE_MIN          = -0.6   // the slowest start (a negative head start = charging from behind)
var INITIATIVE_MAX_SHARE    = 0.5    // never more than this share of the first move's cost

var STAMINA_REGEN           = 2      // stamina per time unit of charging at endurance 10
var REGEN_STAT_WEIGHT       = 0.05
var REGEN_MULT_MIN          = 0.6
var REGEN_MULT_MAX          = 1.5

var EXHAUSTION_THRESHOLD    = 0.2    // stamina fraction at endurance 10 below which a combatant is exhausted
var EXHAUSTION_PER_POINT    = 0.01   // the threshold drops this much per point of endurance over 10
var EXHAUSTION_MIN          = 0.1
var EXHAUSTION_MAX          = 0.3

var FLEE_BASE_CHANCE        = 0.6
var FLEE_PER_POINT          = 0.05   // per point of the party's average agility over the fastest enemy's
var FLEE_CHANCE_MIN         = 0.2
var FLEE_CHANCE_MAX         = 0.95

// Set true to print every roll's breakdown to the console while tuning.
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
    // 0..1 share of the attack's damage that still lands (CombatResolver scales Arbiter.hit's amount by it).
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

// The outcome of one attack's damage roll, before any parry scales it.
class HitResult {
    construct new(amount, crit) {
        _amount = amount
        _crit = crit
    }

    // Damage the attack would deal unparried.
    amount { _amount }
    crit { _crit }
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

    // attacker / defender: Combatants, move: the offensive Move landing. Returns a HitResult.
    static hit(attacker, defender, move) {
        var statMult = Arbiter.damageMult_(attacker, defender)
        var variance = 1 + (RNG.float() - 0.5) * DAMAGE_VARIANCE
        var critChance = Arbiter.critChance(attacker)
        var crit = RNG.float() < critChance
        var amount = (move.damage * statMult * variance * (crit ? CRIT_MULTIPLIER : 1)).round
        if (amount < 1) {
            amount = 1
        }
        if (LOG_ROLLS) {
            System.print("[Arbiter] hit %(attacker.name) -> %(defender.name) %(move.name): base %(move.damage) stat x%(Arbiter.fmt_(statMult)) var x%(Arbiter.fmt_(variance)) crit %(crit) (%(Arbiter.fmt_(critChance))) = %(amount)")
        }
        return HitResult.new(amount, crit)
    }

    // Chance `attacker` lands a critical hit.
    static critChance(attacker) {
        return Math.clamp(CRIT_BASE_CHANCE + (attacker.stats.precision - 10) * CRIT_CHANCE_PER_POINT, 0, CRIT_CHANCE_MAX)
    }

    static damageMult_(attacker, defender) {
        return Math.clamp(1 + (attacker.stats.might - defender.stats.guard) * DAMAGE_STAT_WEIGHT,
                          DAMAGE_MULT_MIN, DAMAGE_MULT_MAX)
    }

    // --- evade ------------------------------------------------------------------------------

    // Chance `defender` slips `attacker`'s unparried attack. defenderMove: the Move the defender is charging
    // (null if none) - Dodge adds DODGE_EVADE_BONUS.
    static evadeChance(attacker, defender, defenderMove) {
        var chance = Math.clamp(EVADE_BASE_CHANCE + (defender.stats.agility - attacker.stats.precision) * EVADE_PER_POINT,
                                0, EVADE_CHANCE_MAX)
        if (defenderMove != null && defenderMove.name == "Dodge") {
            chance = chance + DODGE_EVADE_BONUS
        }
        if (defender.stats.exhausted) {
            chance = chance * EVADE_EXHAUSTED_MULT
        }
        return chance
    }

    // Rolls it: true when the attack misses.
    static evade(attacker, defender, defenderMove) {
        var chance = Arbiter.evadeChance(attacker, defender, defenderMove)
        var evaded = RNG.float() < chance
        if (LOG_ROLLS) {
            System.print("[Arbiter] evade %(defender.name) vs %(attacker.name): chance %(Arbiter.fmt_(chance)) -> %(evaded)")
        }
        return evaded
    }

    // Average damage `attacker`'s `move` would deal `defender` (crits and evasion folded in; no parry, no
    // rolls) - for Combat/EnemyAI.wren to weigh options with.
    static expectedHit(attacker, defender, move, defenderMove) {
        if (!move.offensive) {
            return 0
        }
        var critFactor = 1 + Arbiter.critChance(attacker) * (CRIT_MULTIPLIER - 1)
        var hitFactor = 1 - Arbiter.evadeChance(attacker, defender, defenderMove)
        return move.damage * Arbiter.damageMult_(attacker, defender) * critFactor * hitFactor
    }

    // Time units a landed hit pushes the target's charge back. dealt: damage that actually got through;
    // targetFraction: 0..1, how charged the target's own move was.
    static stagger(attacker, defender, move, dealt, targetFraction) {
        var statMult = Math.clamp(1 + (attacker.stats.might - defender.stats.poise) * STAGGER_STAT_WEIGHT,
                                  STAGGER_MULT_MIN, STAGGER_MULT_MAX)
        var units = Disruption.delay(move, dealt, targetFraction, statMult)
        if (LOG_ROLLS) {
            System.print("[Arbiter] stagger %(attacker.name) -> %(defender.name): dealt %(dealt) charged %(Arbiter.fmt_(targetFraction)) stat x%(Arbiter.fmt_(statMult)) = %(Arbiter.fmt_(units))u")
        }
        return units
    }

    // HP a heal move restores to `actor` (0 for anything that doesn't heal).
    static heal(actor, move) {
        if (move.healAmount <= 0) {
            return 0
        }
        var statMult = Math.clamp(1 + (actor.stats.focus - 10) * HEAL_STAT_WEIGHT, HEAL_MULT_MIN, HEAL_MULT_MAX)
        return (move.healAmount * statMult).round
    }

    // --- tempo ------------------------------------------------------------------------------

    // Effective time-unit cost of `combatant` starting `move` right now.
    static moveCost(combatant, move) {
        if (move.name == "Stalled") {
            return move.timeCost
        }
        var mult = Math.clamp(1 - (combatant.stats.agility - 10) * CHARGE_STAT_WEIGHT, CHARGE_MULT_MIN, CHARGE_MULT_MAX)
        return move.timeCost * mult + (combatant.stats.exhausted ? EXHAUSTION_TIME_PENALTY : 0)
    }

    // Time units of head start (negative = a late start) for `combatant`'s first charge of a fight - rolled
    // once when the fight is set up (Timeline.add); Timeline.commit caps it against the first move's cost.
    static initiative(combatant) {
        var units = (combatant.stats.agility - 10) * INITIATIVE_PER_POINT + RNG.float() * INITIATIVE_ROLL
        if (units < INITIATIVE_MIN) {
            units = INITIATIVE_MIN
        }
        if (LOG_ROLLS) {
            System.print("[Arbiter] initiative %(combatant.name): %(Arbiter.fmt_(units))u")
        }
        return units
    }

    // Caps a rolled head start to INITIATIVE_MAX_SHARE of the first move's effective cost.
    static capInitiative(units, cost) {
        var cap = cost * INITIATIVE_MAX_SHARE
        return units > cap ? cap : units
    }

    // Stamina `combatant` regains over `units` time units of charging.
    static staminaRegen(combatant, units) {
        var mult = Math.clamp(1 + (combatant.stats.endurance - 10) * REGEN_STAT_WEIGHT, REGEN_MULT_MIN, REGEN_MULT_MAX)
        return units * STAMINA_REGEN * mult
    }

    // The stamina fraction below which this Stats bag counts as exhausted (Stats.exhausted).
    static exhaustionThreshold(stats) {
        return Math.clamp(EXHAUSTION_THRESHOLD - (stats.endurance - 10) * EXHAUSTION_PER_POINT,
                          EXHAUSTION_MIN, EXHAUSTION_MAX)
    }

    // --- flee -------------------------------------------------------------------------------

    // party / enemies: the living Combatants on each side.
    static fleeChance(party, enemies) {
        if (party.count == 0 || enemies.count == 0) {
            return 1
        }
        var partyAgility = 0
        for (c in party) {
            partyAgility = partyAgility + c.stats.agility
        }
        partyAgility = partyAgility / party.count
        var fastest = 0
        for (c in enemies) {
            if (c.stats.agility > fastest) {
                fastest = c.stats.agility
            }
        }
        return Math.clamp(FLEE_BASE_CHANCE + (partyAgility - fastest) * FLEE_PER_POINT, FLEE_CHANCE_MIN, FLEE_CHANCE_MAX)
    }

    // Rolls it: true when the party gets away.
    static flee(party, enemies) {
        var chance = Arbiter.fleeChance(party, enemies)
        var escaped = RNG.float() < chance
        if (LOG_ROLLS) {
            System.print("[Arbiter] flee: chance %(Arbiter.fmt_(chance)) -> %(escaped)")
        }
        return escaped
    }

    // An AI defender's stand-in for the player's press timing.
    static aiParryTiming() {
        return AI_TIMING_MIN + RNG.float() * (AI_TIMING_MAX - AI_TIMING_MIN)
    }

    // Two-decimal string for the roll log.
    static fmt_(n) { ((n * 100).round / 100).toString }
}
