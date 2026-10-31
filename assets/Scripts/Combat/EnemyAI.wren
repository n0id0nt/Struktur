// Combat/EnemyAI.wren
// How an AI-controlled combatant chooses: which move and target to commit to (CombatSession.commitEnemyMoves)
// and whether to try parrying an incoming attack (CombatSession.nextParryOffer). The rules those choices play
// out under - and every estimate used to weigh them - come from Combat/Arbiter.wren; this file only decides.
//
// Deterministic, like the rules: the same situation always gets the same choice.
//
// Choosing a move: every affordable (move, target) pair is scored as value per time unit of charge
// (Arbiter.moveCost), so a quick jab and a slow haymaker compete fairly, and the best score wins (ties go to the
// first in kit / party order):
//   attack  - the exact damage it would deal (Arbiter.expectedHit) as a share of the target's max HP, capped at
//             what it has left, + KILL_BONUS if it would finish them, + INTERRUPT_BONUS (scaled by the move's
//             baseDelay) if it lands before the target's own charge completes
//   heal    - HP restored as a share of max, x HEAL_URGENCY below LOW_HP_FRACTION
//   stamina - stamina restored as a share of max x STAMINA_VALUE, x2 when exhausted
// Focus decides how much of that it sees: below KILL_FOCUS it doesn't go for the finishing blow, below
// INTERRUPT_FOCUS it doesn't read the timeline for interrupts - so a brute and a tactician play differently.
// A move costing more stamina than the combatant has is skipped (if nothing is affordable, the cheapest move).
//
// Parry attempts: it knows exactly what a parry would get (Arbiter.previewParry at its reflex-based timing):
//   fail    - never tries
//   perfect - always tries (it loses nothing - its own move survives a perfect parry)
//   partial - tries if it has no move in flight to lose; otherwise only when the hit is worth at least
//             AI_PARRY_THREAT_PIVOT of its current HP and its own move is under AI_PARRY_KEEP_MOVE charged
import "Combat/Arbiter" for Arbiter

var KILL_BONUS                 = 0.5
var INTERRUPT_BONUS            = 0.15   // x move.baseDelay / 4
var LOW_HP_FRACTION            = 0.4
var HEAL_URGENCY               = 2
var STAMINA_VALUE              = 0.5

var KILL_FOCUS                 = 9      // focus needed to value a finishing blow
var INTERRUPT_FOCUS            = 12     // focus needed to aim attacks at interrupting a charge

var AI_PARRY_THREAT_PIVOT      = 0.25   // a partial parry is only worth it against a hit this share of current HP
var AI_PARRY_KEEP_MOVE         = 0.75   // ... and only while its own move is less charged than this

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

        var pick = options[0]
        for (o in options) {
            if (o[2] > pick[2]) {
                pick = o
            }
        }
        return [pick[0], pick[1]]
    }

    // defender: the AI combatant being attacked; attacker / move: the incoming attack. True to attempt a parry
    // (the attempt itself is then judged by Arbiter.parry at the same timing).
    static wantsParry(defender, attacker, move, timeline) {
        // Something with no attacks of its own (the Training Dummy) just stands there and takes it.
        if (!defender.moves.any { |m| m.offensive }) {
            return false
        }
        var tier = Arbiter.previewParry(defender, attacker, Arbiter.aiParryTiming(defender)).tier
        if (tier == "fail") {
            return false
        }
        if (tier == "perfect" || !timeline.isCharging(defender)) {
            return true
        }
        var hp = defender.stats.hp > 0 ? defender.stats.hp : 1
        var threat = Arbiter.expectedHit(attacker, defender, move, timeline.committedMove(defender)) / hp
        return threat >= AI_PARRY_THREAT_PIVOT && timeline.fraction(defender) < AI_PARRY_KEEP_MOVE
    }

    // Value of `enemy` hitting `target` with `move` (cost: its effective time units).
    static attackValue_(enemy, move, target, timeline, cost) {
        var expected = Arbiter.expectedHit(enemy, target, move, timeline.committedMove(target))
        var landed = expected < target.stats.hp ? expected : target.stats.hp
        var value = landed / target.stats.maxHp
        if (enemy.stats.focus >= KILL_FOCUS && expected >= target.stats.hp) {
            value = value + KILL_BONUS
        }
        if (enemy.stats.focus >= INTERRUPT_FOCUS && timeline.isCharging(target) &&
            cost < timeline.costOf(target) - timeline.progress(target)) {
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
