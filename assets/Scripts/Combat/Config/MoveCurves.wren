// Combat/Config/MoveCurves.wren
// Per-move lunge shapes (Combat/Battler.wren chargePos_). Each curve is a "progress" curve evaluated over the
// lunge segment of a move's charge (ActionAnimations.lungeStart .. impact) and fed into a lerp from home to the
// approach point - values above 1 overshoot past it (a lunge), values below 0 pull back before committing (a
// wind-up); the lerp extrapolates fine either way. Every curve ends at 1, so the attacker is at the approach
// point exactly when the hit lands.
// get(id) builds a fresh Curve from CURVES every call - curves are cheap to build and this is a turn-based game,
// so there's no need to cache them.
import "math" for Curve

// curve id -> its keyframes between the two ends, each [time, value]. Curve.new() already starts as a straight
// 0 -> 1 line with keyframes at (0, 0) and (1, 1), so those are left out here - adding them again doubles the end
// keys, which skews the shape and evaluates to nan at t = 0.
var CURVES = {
    // Fast dash in with a small overshoot snap, then settle - a quick, light hit.
    "quickSlash": [[0.25, 1.08]],
    // Double-pulse - jab, pull back, jab again - reads as multiple quick hits.
    "flurryJab": [[0.3, 1.15], [0.55, 0.85], [0.8, 1.1]],
    // Brief wind-up pull-back before a big committed swing forward.
    "heavySwing": [[0.15, -0.08], [0.6, 1.0]],
    // Deeper wind-up, then a hard overshoot slam before settling.
    "heavySlam": [[0.25, -0.15], [0.7, 1.2]],
    // The longest wind-up and the most dramatic overshoot - the biggest hit gets the biggest motion.
    "onslaughtCharge": [[0.3, -0.2], [0.75, 1.3]],
    // Near-instant dart forward, then holds - a snap-dash rather than a wind-up swing.
    "boltDart": [[0.12, 1.0]],
    // A gentle, minimal-overshoot reach - matches Slow's control-archetype feel (a tiny hit that's really about
    // the stagger, not a dramatic lunge).
    "slowPulse": [[0.5, 0.6]]
}

class MoveCurves {
    static get(id) {
        var curve = Curve.new()
        var keyframes = CURVES[id]
        // An unrecognised id gets the bare Curve - a plain linear approach.
        if (keyframes != null) {
            for (key in keyframes) {
                curve.addKeyframe(key[0], key[1])
            }
        }
        return curve
    }
}
