// Combat/EnemyAI.wren
// How an AI-controlled combatant chooses: which move and target to commit to (CombatSession.commitEnemyMoves)
// and whether to try parrying an incoming attack (CombatSession.nextParryOffer). The rules those choices play
// out under - and every estimate used to weigh them - come from Combat/Arbiter.wren; this file only decides.
//
// Choosing a move: every affordable (move, target) pair is scored as value per time unit of charge
// (Arbiter.moveCost), so a quick jab and a slow haymaker compete fairly:
//   attack  - expected damage (Arbiter.expectedHit) as a share of the target's max HP, capped at what it has
//             left, + KILL_BONUS if it would likely finish them, + INTERRUPT_BONUS (scaled by the move's
//             baseDelay) if it lands before the target's own charge completes
//   heal    - HP restored as a share of max, x HEAL_URGENCY below LOW_HP_FRACTION
//   stamina - stamina restored as a share of max x STAMINA_VALUE, x2 when exhausted
// A move costing more stamina than the combatant has is skipped (if nothing is affordable, the cheapest move).
// Smartness - the chance it takes the best-scored option rather than a random one - comes from focus.
//
// Parry attempts: chance = AI_PARRY_BASE + (reflex - attacker precision) x AI_PARRY_PER_POINT
//   + (incoming expected hit / current HP - AI_PARRY_THREAT_PIVOT) x AI_PARRY_THREAT_WEIGHT
//   - own move charged fraction x AI_PARRY_MOVE_LOSS (a parry throws a nearly-ready move away)
//   - AI_PARRY_EXHAUSTED_PENALTY when exhausted, clamped to AI_PARRY_MIN..MAX.
import "math" for Math
import "random" for Random
import "Combat/Arbiter" for Arbiter

var KILL_BONUS                 = 0.5
var INTERRUPT_BONUS            = 0.15   // x move.baseDelay / 4
var LOW_HP_FRACTION            = 0.4
var HEAL_URGENCY               = 2
var STAMINA_VALUE              = 0.5

var SMARTNESS_BASE             = 0.6    // at focus 10
var SMARTNESS_PER_POINT        = 0.04
var SMARTNESS_MIN              = 0.3
var SMARTNESS_MAX              = 0.95

var AI_PARRY_BASE              = 0.5
var AI_PARRY_PER_POINT         = 0.03
var AI_PARRY_THREAT_PIVOT      = 0.25   // an incoming hit worth more than this share of current HP raises the urge
var AI_PARRY_THREAT_WEIGHT     = 0.6
var AI_PARRY_MOVE_LOSS         = 0.3
var AI_PARRY_EXHAUSTED_PENALTY = 0.15
var AI_PARRY_MIN               = 0.1
var AI_PARRY_MAX               = 0.9

var RNG = Random.new()

class EnemyAI {
    // enemy: the Combatant choosing. party: the living Combatants it can attack. timeline: the fight's Timeline
    // (to see what everyone is charging). Returns [move, target] - target is the enemy itself for a move with
    // no victim.
    static choose(enemy, party, timeline) {
        var options = []   // [move, target, score]
        for (m in enemy.moves) {
            if (m.staminaCost > enemy.stats.stamina) {
                continue
            }
            var cost = Arbiter.moveCost(enemy, m)
            if (m.offensive) {
                for (t in party) {
                    options.add([m, t, EnemyAI.attackValue_(enemy, m, t, timeline, cost) / cost])
                }
            } else {
                options.add([m, enemy, EnemyAI.selfValue_(enemy, m) / cost])
            }
        }

        if (options.count == 0) {
            // Too tired for anything: the cheapest move on whoever's first.
            var cheapest = enemy.moves[0]
            for (m in enemy.moves) {
                if (m.staminaCost < cheapest.staminaCost) {
                    cheapest = m
                }
            }
            var t = (cheapest.offensive && party.count > 0) ? party[0] : enemy
            return [cheapest, t]
        }

        var pick = null
        if (RNG.float() < EnemyAI.smartness(enemy)) {
            for (o in options) {
                if (pick == null || o[2] > pick[2]) {
                    pick = o
                }
            }
        } else {
            pick = options[RNG.int(options.count)]
        }
        return [pick[0], pick[1]]
    }

    // Chance this combatant takes its best-scored option (vs a random affordable one).
    static smartness(combatant) {
        return Math.clamp(SMARTNESS_BASE + (combatant.stats.focus - 10) * SMARTNESS_PER_POINT, SMARTNESS_MIN, SMARTNESS_MAX)
    }

    // defender: the AI combatant being attacked; attacker / move: the incoming attack. Rolls the attempt.
    static wantsParry(defender, attacker, move, timeline) {
        // Something with no attacks of its own (the Training Dummy) just stands there and takes it.
        if (!defender.moves.any { |m| m.offensive }) {
            return false
        }
        var chance = AI_PARRY_BASE + (defender.stats.reflex - attacker.stats.precision) * AI_PARRY_PER_POINT
        var hp = defender.stats.hp > 0 ? defender.stats.hp : 1
        var threat = Arbiter.expectedHit(attacker, defender, move, timeline.committedMove(defender)) / hp
        chance = chance + (threat - AI_PARRY_THREAT_PIVOT) * AI_PARRY_THREAT_WEIGHT
        if (timeline.isCharging(defender)) {
            chance = chance - timeline.fraction(defender) * AI_PARRY_MOVE_LOSS
        }
        if (defender.stats.exhausted) {
            chance = chance - AI_PARRY_EXHAUSTED_PENALTY
        }
        chance = Math.clamp(chance, AI_PARRY_MIN, AI_PARRY_MAX)
        return RNG.float() < chance
    }

    // Value of `enemy` hitting `target` with `move` (cost: its effective time units).
    static attackValue_(enemy, move, target, timeline, cost) {
        var expected = Arbiter.expectedHit(enemy, target, move, timeline.committedMove(target))
        var landed = expected < target.stats.hp ? expected : target.stats.hp
        var value = landed / target.stats.maxHp
        if (expected >= target.stats.hp) {
            value = value + KILL_BONUS
        }
        if (timeline.isCharging(target) && cost < timeline.costOf(target) - timeline.progress(target)) {
            value = value + INTERRUPT_BONUS * move.baseDelay / 4
        }
        return value
    }

    // Value of a move `enemy` uses on itself (heal / stamina).
    static selfValue_(enemy, move) {
        var s = enemy.stats
        var value = 0
        if (move.healAmount > 0) {
            var missing = s.maxHp - s.hp
            var healed = Arbiter.heal(enemy, move)
            value = value + (healed < missing ? healed : missing) / s.maxHp * (s.fraction < LOW_HP_FRACTION ? HEAL_URGENCY : 1)
        }
        if (move.staminaRestore > 0) {
            var missing = s.maxStamina - s.stamina
            var restored = move.staminaRestore < missing ? move.staminaRestore : missing
            value = value + restored / s.maxStamina * STAMINA_VALUE * (s.exhausted ? 2 : 1)
        }
        return value
    }
}
