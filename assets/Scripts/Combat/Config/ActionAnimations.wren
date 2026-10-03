// Combat/Config/ActionAnimations.wren
// The catalog of what every action in a fight looks like - one ActionAnimation record per action id.
// Battles are animation driven: a charged move's animation IS its charge (the clip is retimed to the
// move's effective cost x Combat/Timeline.SECONDS_PER_TIME_UNIT and the hit lands on its last frame, the
// moment the bar fills), and every reaction (hurt, parry, fumble, defeat, flee...) is a short timed
// overlay. Combat/Battler.wren plays these; nothing else in a fight owns an animation duration.
//
// There is no battle-specific art yet, so `role` names a clip of the actor's own sheet (the `anims` table
// of Combat/Config/BattlePlayer.wren / BattleCritter.wren: "idle" "attack" "hurt" "run") and the rest of
// the look comes from code-driven extras (a lunge/retreat motion, a tint, a knock-back hop, a particle
// burst). Swap in real clips by pointing `role` at new anims entries - nothing else changes.
import "math" for Vec4

// How long the attacker takes to ease back to its mark after a move lands (Combat/Battler.recover).
var RECOVER_TIME = 0.25

var STALL_TINT = Vec4.new(150, 150, 175, 255)
var HURT_TINT  = Vec4.new(255, 130, 130, 255)

class ActionAnimation {
    // role: which of the actor's own clips to play ("idle" "attack" "hurt" "run").
    // duration: game seconds, or null for a charged move (= the move's effective charge time).
    // loop: true repeats the clip for as long as the action lasts (idle, stalled).
    // motion: "lunge" (toward the target, shaped by curveId), "retreat" (away from it) or "none" -
    //   active over the last (1 - lungeStart) of a charged move, ending exactly at impact.
    // curveId / particleId: Combat/Config/MoveCurves.wren / MoveParticles.wren ids.
    // tint: flat colour applied while it plays (null = leave the sprite alone).
    // hop: knock-back distance in pixels, away from the opponent (reactions).
    // fade: fade the sprite to transparent over the duration and stay gone (defeat).
    construct new(role, duration, loop, motion, lungeStart, curveId, particleId, tint, hop, fade) {
        init_(role, duration, loop, motion, lungeStart, curveId, particleId, tint, hop, fade, false)
    }

    // fumble: the reaction takes over the battler's position - it stumbles from wherever it was (mid-lunge)
    // back to its home mark, swaying as it goes (the stalled attacker), instead of staying on the move's path.
    construct new(role, duration, loop, motion, lungeStart, curveId, particleId, tint, hop, fade, fumble) {
        init_(role, duration, loop, motion, lungeStart, curveId, particleId, tint, hop, fade, fumble)
    }

    init_(role, duration, loop, motion, lungeStart, curveId, particleId, tint, hop, fade, fumble) {
        _role = role
        _duration = duration
        _loop = loop
        _motion = motion
        _lungeStart = lungeStart
        _curveId = curveId
        _particleId = particleId
        _tint = tint
        _hop = hop
        _fade = fade
        _fumble = fumble
    }

    role { _role }
    duration { _duration }
    loop { _loop }
    motion { _motion }
    lungeStart { _lungeStart }
    curveId { _curveId }
    particleId { _particleId }
    tint { _tint }
    hop { _hop }
    fade { _fade }
    fumble { _fumble }
}

class ActionAnimations {
    // The animation a combatant plays for as long as `move` is committed (its whole charge).
    static forMove(move) {
        var n = move.name
        if (n == "Stalled") {
            return ActionAnimation.new("hurt", null, true, "none", 0, null, null, STALL_TINT, 0, false)
        }
        if (n == "Dodge") {
            return ActionAnimation.new("run", null, false, "retreat", 0.5, null, null, null, 0, false)
        }
        if (n == "Mend") {
            return ActionAnimation.new("attack", null, false, "none", 0, null, "healGlow", null, 0, false)
        }
        return ActionAnimation.new("attack", null, false, "lunge", ActionAnimations.lungeStart_(n),
                                   move.curveId, move.particleId, null, 0, false)
    }

    // Fraction of a charge at which the attacker starts moving - slow heavy moves start earlier so the
    // approach reads as a long committed swing, quick ones dart in at the last moment.
    static lungeStart_(name) {
        if (name == "Slash")     return 0.65
        if (name == "Flurry")    return 0.55
        if (name == "Cleave")    return 0.55
        if (name == "Crush")     return 0.5
        if (name == "Onslaught") return 0.45
        if (name == "Bolt")      return 0.7
        if (name == "Slow")      return 0.6
        return 0.6
    }

    // Timed overlays that interrupt whatever a combatant was doing (see Battler.react). "entrance" loops
    // and is ended by Battler.playIdle; "defeat" never ends.
    static reaction(id) {
        if (id == "hurt")     return ActionAnimation.new("hurt", 0.35, false, "none", 0, null, null, HURT_TINT, 10, false)
        // Interrupted mid-charge: reels and fumbles back to its mark, overriding the move's lunge path.
        if (id == "parry")    return ActionAnimation.new("attack", 0.35, false, "none", 0, null, null, null, 0, false)
        // Stalled by a parry: the attacker reels and fumbles back from where its lunge stopped to its mark,
        // overriding the move path (its Stalled look - tinted, looping hurt clip - takes over once it is home).
        if (id == "fumble")   return ActionAnimation.new("hurt", 0.7, false, "none", 0, null, null, null, 12, false, true)
        if (id == "defeat")   return ActionAnimation.new("hurt", 0.6, false, "none", 0, null, null, HURT_TINT, 18, true)
        if (id == "flee")     return ActionAnimation.new("run", 0.7, false, "none", 0, null, null, null, 0, false)
        if (id == "entrance") return ActionAnimation.new("run", null, true, "none", 0, null, null, null, 0, false)
        return ActionAnimation.new("idle", null, true, "none", 0, null, null, null, 0, false)
    }
}
