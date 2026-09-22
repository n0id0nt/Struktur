// Combat/MoveCurves.wren
// Per-move strike-approach shapes (Combat/BattleStage.wren's updateStrike). Each curve is a
// "progress" curve fed into Battler.slide(home, approach, curve.evaluate(t)) - values above 1
// overshoot past the approach point (a lunge), values below 0 pull back before committing (a
// wind-up); slide's plain lerp extrapolates fine either way since it's just from + (to-from)*t.
// get(id) mints a fresh Curve every call, same "getters build a new instance" convention
// Combat/Moves.wren already documents for Move itself - curves are cheap to build and this is a
// turn-based game, so there's no need to cache/memoize them.
import "math" for Curve

class MoveCurves {
    static get(id) {
        if (id == "quickSlash") {
            // Fast dash in with a small overshoot snap, then settle - a quick, light hit.
            var c = Curve.new()
            c.addKeyframe(0, 0)
            c.addKeyframe(0.25, 1.08)
            c.addKeyframe(1, 1)
            return c
        }
        if (id == "flurryJab") {
            // Double-pulse - jab, pull back, jab again - reads as multiple quick hits.
            var c = Curve.new()
            c.addKeyframe(0, 0)
            c.addKeyframe(0.3, 1.15)
            c.addKeyframe(0.55, 0.85)
            c.addKeyframe(0.8, 1.1)
            c.addKeyframe(1, 1)
            return c
        }
        if (id == "heavySwing") {
            // Brief wind-up pull-back before a big committed swing forward.
            var c = Curve.new()
            c.addKeyframe(0, 0)
            c.addKeyframe(0.15, -0.08)
            c.addKeyframe(0.6, 1.0)
            c.addKeyframe(1, 1)
            return c
        }
        if (id == "heavySlam") {
            // Deeper wind-up, then a hard overshoot slam before settling.
            var c = Curve.new()
            c.addKeyframe(0, 0)
            c.addKeyframe(0.25, -0.15)
            c.addKeyframe(0.7, 1.2)
            c.addKeyframe(1, 1)
            return c
        }
        if (id == "onslaughtCharge") {
            // The longest wind-up and the most dramatic overshoot - the biggest hit gets the
            // biggest motion.
            var c = Curve.new()
            c.addKeyframe(0, 0)
            c.addKeyframe(0.3, -0.2)
            c.addKeyframe(0.75, 1.3)
            c.addKeyframe(1, 1)
            return c
        }
        if (id == "boltDart") {
            // Near-instant dart forward, then holds - a snap-dash rather than a wind-up swing.
            var c = Curve.new()
            c.addKeyframe(0, 0)
            c.addKeyframe(0.12, 1.0)
            c.addKeyframe(1, 1)
            return c
        }
        if (id == "slowPulse") {
            // A gentle, minimal-overshoot reach - matches Slow's control-archetype feel (a tiny
            // hit that's really about the stagger, not a dramatic lunge).
            var c = Curve.new()
            c.addKeyframe(0, 0)
            c.addKeyframe(0.5, 0.6)
            c.addKeyframe(1, 1)
            return c
        }
        // Fallback for any unrecognised id - plain linear approach.
        return Curve.new()
    }
}
