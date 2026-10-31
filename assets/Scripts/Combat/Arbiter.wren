// Combat/Arbiter.wren
// The one place a contest between two combatants is decided - every stat comparison in a fight routes through
// here, so tuning the whole battle model means editing this file (and the attribute blocks in Combat/Stats.wren /
// Combat/Config/Archetype.wren). Combat/Disruption.wren stays the raw stagger formula; stagger() here feeds it the
// damage actually dealt and the might-vs-poise multiplier.
//
// Fully deterministic: there is no chance anywhere in battle. The same fighters, moves and timing always produce
// the same result, so every outcome can be read off the numbers (and predicted by Combat/EnemyAI.wren).
//
// Decided here so far (CombatResolver.finishResolve asks, Combatant.use applies):
//   parry   - how well a parry attempt worked: perfect / partial / fail (ParryResult below)
//   evade   - whether an unparried attack misses outright: only while Dodging, agility vs precision
//   hit     - an attack's damage before any parry: might vs guard, x CRIT_MULTIPLIER on an exhausted target
//             (HitResult below)
//   stagger - how far a landed hit shoves the target's charge back: dealt damage + might vs poise
//   heal    - how much a heal restores: focus
// Tempo (Combat/Timeline.wren, Combat/Stats.wren and CombatSession ask):
//   moveCost       - a move's effective charge in time units: agility + the exhaustion penalty
//   initiative     - head start on a combatant's first charge of the fight: agility
//   staminaRegen   - stamina regained per stretch of charging: endurance
//   exhaustion     - the stamina fraction below which a combatant counts as exhausted: endurance
// Escape (CombatResolver.flee asks):
//   flee           - whether this attempt gets the party away: party agility vs the fastest enemy's
// Predictions for decision-making (Combat/EnemyAI.wren - the AI's choices live there, the rules here):
//   expectedHit    - exactly what an attack would deal (crit / evade included), before any parry
//   previewParry   - the tier a parry would get, without recording anything
//
// --- Parry -------------------------------------------------------------------------------------------
//   score = timing + clamp((defender reflex - attacker precision) x PARRY_STAT_WEIGHT, +-PARRY_STAT_EDGE_MAX)
//           - (defender exhausted ? PARRY_EXHAUSTED_PENALTY : 0)
//     timing - 0..1, how far through the parry window the defender committed (0 = the instant the prompt
//              opened, 1 = at impact). Waiting longer reads the attack better - but if the window runs out
//              there is no parry at all (ParryPromptState). AI defenders time it by reflex (aiParryTiming).
//   score >= PARRY_PERFECT_SCORE -> perfect: no damage, attacker stalled, defender keeps its own move
//   score >= PARRY_PARTIAL_SCORE -> partial: PARTIAL_DAMAGE_THROUGH of the damage, defender's move lost
//   otherwise                    -> fail:    full damage, defender's move lost (it dropped it to guard)
// Decided when the defender commits (so the parry pose plays at once) and applied at impact
// (CombatResolver.finishResolve).
//
// --- Evade -------------------------------------------------------------------------------------------
//   An unparried attack misses outright when the defender is charging Dodge, isn't exhausted, and its agility
//   is at least the attacker's precision. Nothing else ever misses. An evaded attack deals nothing and
//   staggers nothing.
//
// --- Hit ---------------------------------------------------------------------------------------------
//   amount = max(1, round(move damage x clamp(1 + (might - guard) x DAMAGE_STAT_WEIGHT, DAMAGE_MULT_MIN..MAX)
//                          x (crit ? CRIT_MULTIPLIER : 1)))
//   crit: the target is exhausted (below its exhaustion line) - an opening, not luck.
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
//   initiative = max((agility - 10) x INITIATIVE_PER_POINT, INITIATIVE_MIN) time units, capped at
//                INITIATIVE_MAX_SHARE of the first move's cost - negative = a late start.
//   regen      = units x STAMINA_REGEN x clamp(1 + (endurance - 10) x REGEN_STAT_WEIGHT, REGEN_MULT_MIN..MAX)
//   exhaustion = clamp(EXHAUSTION_THRESHOLD - (endurance - 10) x EXHAUSTION_PER_POINT, EXHAUSTION_MIN..MAX)
//
// --- Flee --------------------------------------------------------------------------------------------
//   attempts needed = 1 + ceil(max(0, fastest living enemy agility - average living party agility)
//                              / FLEE_AGILITY_PER_ATTEMPT)
//   An as-fast party gets away first time; a slower one has to fail (and lose the turn to a stall) once per
//   FLEE_AGILITY_PER_ATTEMPT points it's behind - but always gets away in the end.
import "math" for Math
import "Combat/Disruption" for Disruption

var PARRY_STAT_WEIGHT       = 0.03   // score per point of reflex over the attacker's precision
var PARRY_STAT_EDGE_MAX     = 0.3    // cap on the stat edge either way, so timing always matters
var PARRY_EXHAUSTED_PENALTY = 0.15   // a defender below Stats' exhaustion threshold parries worse
var PARRY_PERFECT_SCORE     = 0.8
var PARRY_PARTIAL_SCORE     = 0.45
var PARTIAL_DAMAGE_THROUGH  = 0.5    // share of the attack's damage a partial parry lets through
var AI_TIMING_BASE          = 0.3    // an AI defender's parry timing at reflex 10 ...
var AI_TIMING_PER_POINT     = 0.05   // ... + this per point of reflex over 10
var AI_TIMING_MIN           = 0.1
var AI_TIMING_MAX           = 0.95

var DAMAGE_STAT_WEIGHT      = 0.03   // damage multiplier per point of attacker might over defender guard
var DAMAGE_MULT_MIN         = 0.6
var DAMAGE_MULT_MAX         = 1.5
var CRIT_MULTIPLIER         = 1.5    // a hit on an exhausted target

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

var FLEE_AGILITY_PER_ATTEMPT = 4     // each this many points the party is slower costs one extra flee attempt

// Set true to print every check's breakdown to the console while tuning.
var LOG_ROLLS = true
// How many short check lines recentRolls keeps (the battle test's on-screen readout shows them).
var RECENT_ROLLS_KEPT = 8

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

    static forScore(score) {
        if (score >= PARRY_PERFECT_SCORE) {
            return ParryResult.perfect(score)
        }
        if (score >= PARRY_PARTIAL_SCORE) {
            return ParryResult.partial(score)
        }
        return ParryResult.fail(score)
    }
}

// The outcome of one attack's damage check, before any parry scales it.
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
    // --- parry ------------------------------------------------------------------------------

    // defender / attacker: Combatants. timing: 0..1 through the parry window (see header). Returns a ParryResult.
    static parry(defender, attacker, timing) {
        var edge = Arbiter.parryEdge_(defender, attacker)
        var penalty = defender.stats.exhausted ? PARRY_EXHAUSTED_PENALTY : 0
        var result = Arbiter.previewParry(defender, attacker, timing)
        Arbiter.record_("Parry  %(defender.name) vs %(attacker.name): t%(Arbiter.fmt_(timing)) edge %(Arbiter.fmt_(edge)) exh -%(Arbiter.fmt_(penalty)) = %(Arbiter.fmt_(result.score)) %(result.tier)",
                        "parry %(defender.name) vs %(attacker.name): timing %(Arbiter.fmt_(timing)) edge %(Arbiter.fmt_(edge)) exhausted -%(Arbiter.fmt_(penalty)) = %(Arbiter.fmt_(result.score)) -> %(result.tier)")
        return result
    }

    // The ParryResult a parry at `timing` would get - nothing recorded (EnemyAI uses this to decide).
    static previewParry(defender, attacker, timing) {
        var penalty = defender.stats.exhausted ? PARRY_EXHAUSTED_PENALTY : 0
        return ParryResult.forScore(Math.clamp(timing, 0, 1) + Arbiter.parryEdge_(defender, attacker) - penalty)
    }

    // An AI defender's stand-in for the player's press timing: quicker reflexes read the attack later and better.
    static aiParryTiming(defender) {
        return Math.clamp(AI_TIMING_BASE + (defender.stats.reflex - 10) * AI_TIMING_PER_POINT, AI_TIMING_MIN, AI_TIMING_MAX)
    }

    static parryEdge_(defender, attacker) {
        return Math.clamp((defender.stats.reflex - attacker.stats.precision) * PARRY_STAT_WEIGHT,
                          -PARRY_STAT_EDGE_MAX, PARRY_STAT_EDGE_MAX)
    }

    // --- hit --------------------------------------------------------------------------------

    // attacker / defender: Combatants, move: the offensive Move landing. Returns a HitResult.
    static hit(attacker, defender, move) {
        var statMult = Arbiter.damageMult_(attacker, defender)
        var crit = Arbiter.crits(defender)
        var amount = Arbiter.amount_(attacker, defender, move)
        Arbiter.record_("Hit  %(attacker.name) > %(defender.name) %(move.name): %(move.damage) x%(Arbiter.fmt_(statMult))%(crit ? " CRIT" : "") = %(amount)",
                        "hit %(attacker.name) -> %(defender.name) %(move.name): base %(move.damage) stat x%(Arbiter.fmt_(statMult)) crit %(crit) (target exhausted) = %(amount)")
        return HitResult.new(amount, crit)
    }

    // True when a hit on `defender` lands as a crit - it's exhausted, so it can't brace for the blow.
    static crits(defender) { defender.stats.exhausted }

    static damageMult_(attacker, defender) {
        return Math.clamp(1 + (attacker.stats.might - defender.stats.guard) * DAMAGE_STAT_WEIGHT,
                          DAMAGE_MULT_MIN, DAMAGE_MULT_MAX)
    }

    static amount_(attacker, defender, move) {
        var amount = (move.damage * Arbiter.damageMult_(attacker, defender) * (Arbiter.crits(defender) ? CRIT_MULTIPLIER : 1)).round
        return amount < 1 ? 1 : amount
    }

    // --- evade ------------------------------------------------------------------------------

    // True when `defender` slips `attacker`'s unparried attack. defenderMove: the Move the defender is charging
    // (null if none) - only Dodge evades, and only with agility to match the attacker's precision.
    static evades(attacker, defender, defenderMove) {
        return defenderMove != null && defenderMove.name == "Dodge" && !defender.stats.exhausted &&
               defender.stats.agility >= attacker.stats.precision
    }

    // Checks it and records the result: true when the attack misses.
    static evade(attacker, defender, defenderMove) {
        var evaded = Arbiter.evades(attacker, defender, defenderMove)
        var dodging = defenderMove != null && defenderMove.name == "Dodge"
        Arbiter.record_("Evade  %(defender.name): %(evaded ? "EVADED" : (dodging ? "dodge too slow - hit" : "not dodging - hit"))",
                        "evade %(defender.name) vs %(attacker.name): dodging %(dodging) agility %(defender.stats.agility) vs precision %(attacker.stats.precision) exhausted %(defender.stats.exhausted) -> %(evaded)")
        return evaded
    }

    // Exactly what `attacker`'s `move` would deal `defender` right now, before any parry (0 if it would be
    // evaded) - for Combat/EnemyAI.wren to weigh options with.
    static expectedHit(attacker, defender, move, defenderMove) {
        if (!move.offensive || Arbiter.evades(attacker, defender, defenderMove)) {
            return 0
        }
        return Arbiter.amount_(attacker, defender, move)
    }

    // --- stagger / heal ---------------------------------------------------------------------

    // Time units a landed hit pushes the target's charge back. dealt: damage that actually got through;
    // targetFraction: 0..1, how charged the target's own move was.
    static stagger(attacker, defender, move, dealt, targetFraction) {
        var statMult = Math.clamp(1 + (attacker.stats.might - defender.stats.poise) * STAGGER_STAT_WEIGHT,
                                  STAGGER_MULT_MIN, STAGGER_MULT_MAX)
        var units = Disruption.delay(move, dealt, targetFraction, statMult)
        Arbiter.record_("Stagger  %(defender.name): %(dealt) dmg x%(Arbiter.fmt_(statMult)) = -%(Arbiter.fmt_(units))u",
                        "stagger %(attacker.name) -> %(defender.name): dealt %(dealt) charged %(Arbiter.fmt_(targetFraction)) stat x%(Arbiter.fmt_(statMult)) = %(Arbiter.fmt_(units))u")
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

    // Time units of head start (negative = a late start) for `combatant`'s first charge of a fight - worked out
    // once when the fight is set up (Timeline.add); Timeline.commit caps it against the first move's cost.
    static initiative(combatant) {
        var units = (combatant.stats.agility - 10) * INITIATIVE_PER_POINT
        if (units < INITIATIVE_MIN) {
            units = INITIATIVE_MIN
        }
        Arbiter.record_("Initiative  %(combatant.name): %(Arbiter.fmt_(units))u",
                        "initiative %(combatant.name): agility %(combatant.stats.agility) = %(Arbiter.fmt_(units))u")
        return units
    }

    // Caps a head start to INITIATIVE_MAX_SHARE of the first move's effective cost.
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

    // How many attempts the party needs to get away (see header). party / enemies: the living Combatants.
    static fleeAttemptsNeeded(party, enemies) {
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
        var behind = fastest - partyAgility
        return behind <= 0 ? 1 : 1 + (behind / FLEE_AGILITY_PER_ATTEMPT).ceil
    }

    // True when this attempt gets away. attempt: 1 for the first try this fight, 2 for the next... (CombatSession
    // counts them).
    static flee(party, enemies, attempt) {
        var needed = Arbiter.fleeAttemptsNeeded(party, enemies)
        var escaped = attempt >= needed
        Arbiter.record_("Flee: try %(attempt) of %(needed) %(escaped ? "escaped" : "failed")",
                        "flee: attempt %(attempt), needs %(needed) -> %(escaped)")
        return escaped
    }

    // --- check log ------------------------------------------------------------------------

    // Every check lands here: its full breakdown to the console (when LOG_ROLLS is on) and a short line into
    // recentRolls - what the battle test (States/BattleTestState.wren) shows on screen.
    static record_(short, long) {
        if (LOG_ROLLS) {
            System.print("[Arbiter] %(long)")
        }
        if (__recent == null) {
            __recent = []
        }
        __recent.add(short)
        if (__recent.count > RECENT_ROLLS_KEPT) {
            __recent.removeAt(0)
        }
    }

    // The last RECENT_ROLLS_KEPT checks, oldest first, as short one-line summaries.
    static recentRolls { __recent == null ? [] : __recent }

    static clearRolls() {
        __recent = []
    }

    // Two-decimal string for the check log.
    static fmt_(n) { ((n * 100).round / 100).toString }
}
