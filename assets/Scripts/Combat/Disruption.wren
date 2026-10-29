// Combat/Disruption.wren
// The interrupt formula, verbatim from the Time-Based Combat design doc - Phase 3, the vertical
// slice the whole roadmap is built to protect:
//
//   Interrupt Delay = Base Delay x Power Multiplier x Timing Multiplier x Stat Multiplier
//     Base Delay        - the interrupting move's own wind-up (2-6 time units, Move.baseDelay)
//     Power Multiplier  - clamp(damage dealt / 50, 0.5, 2.0)
//     Timing Multiplier - 1.5 - how far (0..1) into their move the target had charged
//     Stat Multiplier   - attacker might vs defender poise, worked out by Combat/Arbiter.stagger
//   Final result clamped to 1-8 time units.
//
// The intent: hitting someone at the START of their wind-up catches them off guard (big setback);
// hitting them near the END barely fazes them, since they're already committed. Stronger attacks
// disrupt harder.
import "math" for Math

var POWER_DIVISOR = 50
var POWER_MIN     = 0.5
var POWER_MAX     = 2
var MIN_DELAY     = 1
var MAX_DELAY     = 8

class Disruption {
    // move: the Move that just landed. damage: what it actually dealt. targetFraction: 0..1, how charged the
    // target's own move was. statMultiplier: Combat/Arbiter.stagger's might-vs-poise factor (1 = neutral).
    // Returns the number of time units to push the target's charge backward.
    static delay(move, damage, targetFraction, statMultiplier) {
        var power  = Math.clamp(damage / POWER_DIVISOR, POWER_MIN, POWER_MAX)
        var timing = 1.5 - Math.clamp(targetFraction, 0, 1)
        return Math.clamp(move.baseDelay * power * timing * statMultiplier, MIN_DELAY, MAX_DELAY)
    }
}
