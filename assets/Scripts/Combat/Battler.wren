// Combat/Battler.wren
// One combatant as it exists in the arena: a spawned entity wearing a def's sprite + animations
// (Combat/Config/BattleCritter.wren for enemies, Combat/Config/BattlePlayer.wren for the player),
// plus the Combatant it fights as. Built and owned by Combat/BattleStage.wren, torn down with the
// fight.
//
// Battles are animation driven (see Combat/Config/ActionAnimations.wren for the data):
//   - While a move is committed the battler plays that move's clip, retimed to the move's charge time and
//     re-anchored to the Timeline fraction every frame (update) - so the clip, the lunge and the charge
//     bar are one thing, and slowing/pausing game time (Time.setTimeScale) slows/pauses all of it.
//   - With no move committed (the player while choosing) it idles.
//   - Reactions (hurt / parry / fumble / defeat / flee) are timed overlays that interrupt either, then fall
//     back to whichever applies.
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, Sprite, SpriteAnimation, ParticleEmitter, RenderLayer
import "math" for Vec3, Vec4
import "resourceManager" for Texture
import "animation" for SpriteAnimationDefinition
import "renderer" for FlipBit
import "app" for Time
import "Colors" for WHITE
import "Combat/Timeline" for SECONDS_PER_TIME_UNIT
import "Combat/Config/ActionAnimations" for ActionAnimations
import "Combat/Config/MoveCurves" for MoveCurves
import "Combat/Config/MoveParticles" for MoveParticles

// ATTACK_REACH: how far from home toward the target's home an attacker lunges, as a fraction of that
// distance - close enough to read as a hit without fully overlapping the target's sprite.
var ATTACK_REACH     = 0.6
var RETREAT_DISTANCE = 28   // px a "retreat" move (Dodge) slips back by
var RUN_OFF_DISTANCE = 220  // px the fleeing player slides off-stage by

class Battler {
    // def: a BattleCritter instance or the BattlePlayer class - same getters either way
    //   (name / texturePath / cols / rows / pivot / facing / anims).
    // combatant: the Combatant this battler fights as (the player's is persistent).
    // parent: entity to parent the spawned battler under (the arena anchor).
    construct new(def, combatant, parent) {
        _def = def
        _combatant = combatant
        _entity = GameObject.create("Battler_%(def.name)", parent)

        var tex = Texture.load(def.texturePath)
        Sprite.create(_entity, tex, WHITE, def.pivot, def.cols, def.rows, FlipBit.NONE, 0, RenderLayer.ENTITIES, 0)
        tex.unload()

        _anim = SpriteAnimation.create(_entity)
        _registered = {}   // clip keys already added - SpriteAnimation.addAnimation rejects duplicates

        _home = null
        _base = null          // where the battler stands this frame, before any reaction hop
        _awaySign = 1         // +1 / -1: which way "away from the opponent" is along x
        _recoverFrom = null

        _move = null          // committed move being animated (null = idle)
        _action = null
        _curve = null
        _chargeClip = null

        _reaction = null
        _reactionStart = 0
        _reactionEnd = 0
        _reactionFrom = null   // where the battler stood when the current reaction began (the fumble's start)
        _defeated = false

        playIdle()

        // One persistent particle emitter, reused for every move's impact effect that lands on this
        // battler (playImpact) instead of creating/destroying an emitter per hit - see
        // Combat/Config/MoveParticles.wren. The initial texture is just a placeholder; playImpact
        // swaps it (and everything else) before ever emitting, and ParticleEmitter.create requires
        // some valid texture up front.
        var particleTex = Texture.load("Sprites/magic.png")
        _emitter = ParticleEmitter.create(_entity, particleTex)
        particleTex.unload()
        _emitter.looping = false
    }

    entity { _entity }
    combatant { _combatant }

    // Formation slot position - set once by BattleStage right after construction; the lunge starts and
    // ends here.
    home { _home }
    home=(pos) {
        _home = pos
        _base = pos
    }

    awaySign=(sign) { _awaySign = sign }

    // dir: 1 to face right, -1 to face left. Flips the sheet when that's opposite the art's facing.
    face(dir) {
        Sprite.setFlipped(_entity, dir == _def.facing ? FlipBit.NONE : FlipBit.HORIZONTAL)
    }

    // Slide from fromPos toward toPos, t in 0..1 (the arena entrance - see BattleStage.intro).
    slide(fromPos, toPos, t) {
        _base = lerp3_(fromPos, toPos, t)
        WorldTransform.setPosition(_entity, _base)
    }

    // --- clips ---------------------------------------------------------------------

    // The key of the clip for `role` at `duration` seconds (null = the def's own length), registering
    // it on first use. Frame ranges come from the def's anims table; a role the def lacks falls back to
    // idle. loop=false clips hold their last frame.
    clip_(role, duration, loop) {
        var frames = _def.anims.containsKey(role) ? _def.anims[role] : _def.anims["idle"]
        var seconds = duration == null ? frames[2] : duration
        var key = "%(role)|%(seconds)|%(loop)"
        if (!_registered.containsKey(key)) {
            _anim.addAnimation(key, SpriteAnimationDefinition.new(frames[0], frames[1], seconds, loop))
            _registered[key] = true
        }
        return key
    }

    playIdle() {
        SpriteAnimation.setCurrentAnimation(_entity, clip_("idle", null, true))
    }

    // The arena entrance: run in place while BattleStage.intro slides everyone onto their marks.
    playEntrance() {
        SpriteAnimation.forcePlayAnimation(_entity, clip_("run", null, true))
    }

    // --- charging a move -------------------------------------------------------------

    // Starts animating `move`: from now until endCharge the battler plays its clip, retimed to `cost`
    // time units (the move's effective charge time), driven by update().
    beginCharge(move, cost) {
        _move = move
        _action = ActionAnimations.forMove(move)
        _curve = _action.motion == "lunge" ? MoveCurves.get(_action.curveId) : null
        if (_action.loop) {
            _chargeClip = clip_(_action.role, null, true)
        } else {
            _chargeClip = clip_(_action.role, cost * SECONDS_PER_TIME_UNIT, false)
        }
        refreshTint_()
    }

    // The move is gone (resolved, cancelled): back to idle. Position stays where the move left it until
    // recover() eases it home.
    endCharge() {
        _move = null
        _action = null
        _curve = null
        _chargeClip = null
        refreshTint_()
        if (_reaction == null && !_defeated) {
            playIdle()
        }
    }

    // Called every frame by BattleStage.drive. fraction: the Timeline's 0..1 charge of this battler's
    // move. targetHome: where the move's target stands (the lunge heads toward it).
    update(fraction, targetHome) {
        var now = Time.scaledTime
        if (_reaction != null && !_defeated && now >= _reactionEnd) {
            _reaction = null
            refreshTint_()
            if (_move == null) {
                playIdle()
            }
        }

        if (_reaction != null && _reaction.fumble && !_defeated) {
            // A fumble (the parried attacker) overrides the move's path: stumble from wherever the lunge got to back to the mark.
            _base = fumblePos_(now)
        } else if (_move != null && !_defeated) {
            if (_reaction == null) {
                if (_action.loop) {
                    SpriteAnimation.setCurrentAnimation(_entity, _chargeClip)
                } else {
                    SpriteAnimation.playAnimationAt(_entity, _chargeClip, fraction)
                }
            }
            _base = chargePos_(fraction, targetHome)
        }
        place_(now)
    }

    // The fumble's path: eased from where the reaction began (mid-lunge, say) back to the home mark, with
    // a sway that dies away as it settles - a stumble rather than a slide. Ends exactly on the mark.
    fumblePos_(now) {
        var t = (now - _reactionStart) / _reaction.duration
        if (t > 1) {
            t = 1
        }
        var p = smooth_(t)
        var x = _reactionFrom.x + (_home.x - _reactionFrom.x) * p + _awaySign * 5 * (t * Num.pi * 3).sin * (1 - t)
        var y = _reactionFrom.y + (_home.y - _reactionFrom.y) * p + 3 * (t * Num.pi * 5).sin * (1 - t)
        return Vec3.new(x, y, 0)
    }

    // Where the committed move puts the battler at this fraction of its charge. A move with no motion
    // (Mend, Stalled) leaves it wherever it is, so a battler still recovering from its last move isn't
    // snapped home.
    chargePos_(fraction, targetHome) {
        if (_action.motion == "none") {
            return _base
        }
        if (targetHome == null || fraction <= _action.lungeStart) {
            return _home
        }
        var t = (fraction - _action.lungeStart) / (1 - _action.lungeStart)
        if (_action.motion == "lunge") {
            var approach = lerp3_(_home, targetHome, ATTACK_REACH)
            return lerp3_(_home, approach, _curve.evaluate(t))
        }
        return Vec3.new(_home.x + _awaySign * RETREAT_DISTANCE * smooth_(t), _home.y, 0)
    }

    // --- reactions ---------------------------------------------------------------------

    // Plays a timed overlay from ActionAnimations.reaction(id) over whatever the battler was doing.
    // "defeat" never ends (the battler fades out and stays down).
    react(id) {
        if (_defeated) {
            return
        }
        var a = ActionAnimations.reaction(id)
        _reaction = a
        _reactionStart = Time.scaledTime
        _reactionFrom = _base
        _reactionEnd = _reactionStart + a.duration
        SpriteAnimation.forcePlayAnimation(_entity, clip_(a.role, a.duration, false))
        if (id == "defeat") {
            _defeated = true
        }
        refreshTint_()
    }

    // The battler's position this frame: its base spot plus any reaction hop (knock-back out and back,
    // or - for a defeat - out and staying out), and the defeat fade.
    place_(now) {
        var offset = 0
        if (_reaction != null) {
            var t = _reaction.duration > 0 ? (now - _reactionStart) / _reaction.duration : 1
            if (t > 1) {
                t = 1
            }
            if (_reaction.hop != 0) {
                offset = _awaySign * _reaction.hop * (_defeated ? smooth_(t) : (t * Num.pi).sin)
            }
            if (_defeated && _reaction.fade) {
                var c = _reaction.tint
                setColor_(Vec4.new(c.x, c.y, c.z, 255 * (1 - t)))
            }
        }
        WorldTransform.setPosition(_entity, Vec3.new(_base.x + offset, _base.y, 0))
    }

    // --- recovering after a move lands ------------------------------------------------

    // True when the last move left this battler away from its mark (a lunge / retreat).
    // A battler mid-fumble is already on its way home by itself, so it never needs a recover.
    needsRecover {
        if (_reaction != null && _reaction.fumble) {
            return false
        }
        return _base != null && _home != null && ((_base.x - _home.x).abs + (_base.y - _home.y).abs) > 0.5
    }

    beginRecover() {
        _recoverFrom = _base
    }

    // t: 0..1 progress easing the battler from where the move left it back to its mark.
    recover(t) {
        _base = lerp3_(_recoverFrom, _home, smooth_(t))
        place_(Time.scaledTime)
    }

    // t: 0..1 progress sliding the battler off-stage, away from the opponents (the "flee" run-off).
    runOff(t) {
        _base = Vec3.new(_home.x + _awaySign * RUN_OFF_DISTANCE * smooth_(t), _home.y, 0)
        place_(Time.scaledTime)
    }

    // Reactions or the stalled look may recolour the sprite; this picks the right colour for the current
    // state (a reaction's tint, else the committed action's, else none).
    refreshTint_() {
        if (_defeated) {
            return
        }
        if (_reaction != null && _reaction.tint != null) {
            setColor_(_reaction.tint)
        } else if (_action != null && _action.tint != null) {
            setColor_(_action.tint)
        } else {
            setColor_(WHITE)
        }
    }

    setColor_(color) {
        var sprite = Sprite.get(_entity)
        if (sprite != null) {
            sprite.color = color
        }
    }

    // Reconfigures this battler's persistent particle emitter for particleId and fires it - called at the
    // moment a hit / parry / heal lands on this battler (BattleStage.playImpact).
    playImpact(particleId) {
        MoveParticles.trigger(_emitter, particleId)
    }

    place(pos) {
        _base = pos
        WorldTransform.setPosition(_entity, pos)
    }

    teardown() {
        if (GameObject.isValid(_entity)) {
            GameObject.destroy(_entity)
        }
    }

    lerp3_(a, b, t) { Vec3.new(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, 0) }

    smooth_(t) {
        var x = t < 0 ? 0 : (t > 1 ? 1 : t)
        return x * x * (3 - 2 * x)
    }
}
