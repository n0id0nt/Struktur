// Combat/Battler.wren
// One combatant as it exists in the arena: a spawned entity wearing a def's sprite + animations
// (Combat/Config/BattleCritter.wren, or Combat/Config/BattlePlayer.wren for the player), plus the Combatant it shows.
// Built and owned by Combat/BattleStage.wren, torn down with the fight.
//
// Battles are animation driven. Every frame the battler reads its Combatant (Combat/BattleEngine.wren moves it on):
//   - a move in progress plays that move's clip, retimed to the move's length and anchored to how far into it the
//     combatant is - so the clip, the hit and the timeline are one thing, and pausing game time freezes all of it;
//   - a combatant in a damage animation plays its hurt clip; an idle or holding one idles;
//   - the sprite slides to the combatant's cell whenever that changes, leans into an attack around its impact, and
//     is tinted by what it is doing: guarding (blue, strongest at the peak), dodging (ghosted - Soul Step turns it
//     into flickering soul light, pouring off sparks: Util/SoulFx.wren), Exposed (gold),
//     staggered (red), burning / chilled.
// A def may carry a weapon layer (def.weaponTexturePath): a second sheet with the same grid, drawn over the body
// frame for frame. Every hit that lands on it throws up a cloud of dust coloured by the blow's element
// (playHitCloud), from its own emitter on a child entity; damage, heals and call-outs float up off it as text
// particles (popText), from another.
import "gameObject" for GameObject
import "gameObjectComponents" for WorldTransform, LocalTransform, Sprite, SpriteAnimation, ParticleEmitter, RenderLayer
import "math" for Math, Vec2, Vec3, Vec4
import "resourceManager" for Texture, Font
import "animation" for SpriteAnimationDefinition
import "renderer" for FlipBit
import "app" for Time
import "Util/Colors" for WHITE
import "Combat/Config/SheetAnims" for SheetAnims
import "Combat/Config/MoveParticles" for MoveParticles
import "Util/SoulFx" for SoulFx

var SLIDE_SPEED = 260        // px per second between cells
var LEAN = 7                 // px an attack leans toward its target at impact
var HURT_TINT = Vec4.new(255, 120, 120, 255)
var GUARD_TINT = Vec4.new(120, 170, 255, 255)
var EXPOSED_TINT = Vec4.new(255, 225, 110, 255)
var BURN_TINT = Vec4.new(255, 170, 90, 255)
var CHILL_TINT = Vec4.new(150, 215, 255, 255)
var DEFEAT_TIME = 0.9
// Soul Step's untouchable moment: the move's name, how many soul sparks a second pour off the battler, and how high
// (px above its feet) they rise from.
var SOUL_STEP = "Soul Step"
var SOUL_SPARK_RATE = 45
var SOUL_SPARK_HEIGHT = 20
var CLOUD_HEIGHT = 8         // px above the feet that a hit cloud bursts from (low enough for a small wolf)
var CLOUD_FULL_DAMAGE = 40   // a hit this hard (or harder) throws up the biggest cloud
var TEXT_HEIGHT = 46         // px above the feet that floating text starts from (clear of the player's head)
var TEXT_FONT = "UI/Font/NormalFont.ttf"   // the game's pixel font (9 px design size)

class Battler {
    // def: a BattleCritter or the BattlePlayer class - same getters either way. combatant: who it shows.
    // parent: entity to parent the spawned battler under. position: where it starts (a Vec3).
    construct new(def, combatant, parent, position) {
        _def = def
        _combatant = combatant
        _entity = GameObject.create("Battler_%(def.name)", parent)

        var tex = Texture.load(def.texturePath)
        Sprite.create(_entity, tex, WHITE, def.pivot, def.cols, def.rows, FlipBit.NONE, 0, RenderLayer.ENTITIES, 0)
        tex.unload()
        _anim = SpriteAnimation.create(_entity)
        _registered = {}   // clip keys already added - SpriteAnimation.addAnimation rejects duplicates

        _pos = position
        _target = position
        _side = 1
        _clip = null
        _wasStunned = false
        _defeatedAt = null
        _gone = false
        _hop = 0
        _hopUntil = 0
        _base = def.tint == null ? WHITE : def.tint
        _lastMove = null
        WorldTransform.setPosition(_entity, position)

        // The weapon layer: its own sprite on a child entity, kept on the body's frame every update.
        _weapon = null
        if (Battler.hasWeaponLayer_(def) && combatant.loadout.main != null) {
            _weapon = GameObject.create("Weapon_%(def.name)", _entity)
            var weaponTex = Texture.load(def.weaponTexturePath)
            Sprite.create(_weapon, weaponTex, WHITE, def.pivot, def.cols, def.rows, FlipBit.NONE, 0, RenderLayer.ENTITIES, 1)
            weaponTex.unload()
        }

        // One persistent particle emitter, reused for every effect that lands on this battler (playImpact) - see
        // Combat/Config/MoveParticles.wren. The initial texture is just a placeholder.
        var particleTex = Texture.load("Actors/magic.png")
        _emitter = ParticleEmitter.create(_entity, particleTex)
        particleTex.unload()
        _emitter.looping = false

        // The hit cloud's emitter: a child a little above the feet (where a blow lands), kept apart from the impact
        // sparks so neither restyles the other's particles mid-flight.
        _cloud = GameObject.create("HitCloud_%(def.name)", _entity)
        LocalTransform.setPosition(_cloud, Vec3.new(0, -CLOUD_HEIGHT, 0))
        var cloudTex = Texture.load("Actors/magic.png")
        _cloudEmitter = ParticleEmitter.create(_cloud, cloudTex)
        cloudTex.unload()
        _cloudEmitter.looping = false

        // Floating text (popText): text particles that rise and fade over the battler's head. Each carries its own
        // text and colour, so a quick second number never restyles the first.
        _text = GameObject.create("FloatText_%(def.name)", _entity)
        LocalTransform.setPosition(_text, Vec3.new(0, -TEXT_HEIGHT, 0))
        var textTex = Texture.load("Actors/magic.png")   // unused by text particles - create() needs one
        _textEmitter = ParticleEmitter.create(_text, textTex)
        textTex.unload()
        _textFont = Font.load(TEXT_FONT, 9, true)
        _textEmitter.font = _textFont
        _textEmitter.fontSize = 9
        _textEmitter.outlineColor = Vec4.new(19, 27, 27, 255)
        _textEmitter.looping = false
        _textEmitter.burstCount = 0
        _textEmitter.maxParticles = 12
        _textEmitter.spawnRadius = 3
        _textEmitter.lifetimeMin = 1.1
        _textEmitter.lifetimeMax = 1.25
        _textEmitter.velocityMin = Vec2.new(-6, -34)
        _textEmitter.velocityMax = Vec2.new(6, -26)
        _textEmitter.acceleration = Vec2.new(0, 22)   // slows as it rises
        _textEmitter.startScale = 1
        _textEmitter.endScale = 1
        _textEmitter.rotationSpeedMin = 0
        _textEmitter.rotationSpeedMax = 0

        face(1)
        play_("idle", null, true)
    }

    // Only the player's sheet has a weapon layer so far (Combat/Config/BattlePlayer.weaponTexturePath).
    static hasWeaponLayer_(def) {
        return def.name == "You" && def.weaponTexturePath != null
    }

    entity { _entity }
    combatant { _combatant }
    position { _pos }
    gone { _gone }

    // Where the battler should stand; it slides there.
    target=(pos) { _target = pos }
    // Jumps straight to `pos` (the entrance slide writes the position itself).
    place(pos) {
        _pos = pos
        _target = pos
        WorldTransform.setPosition(_entity, pos)
    }

    // Draw order within the arena: battlers in nearer lanes over those behind.
    order=(value) {
        Sprite.setOrderInLayer(_entity, value)
        if (_weapon != null) Sprite.setOrderInLayer(_weapon, value + 1)
        // Just over this battler (and its weapon), still under anyone standing nearer the camera.
        _cloudEmitter.orderInLayer = value + 2
        // Text reads over everyone in the arena.
        _textEmitter.orderInLayer = 1000 + value
    }

    // dir: 1 to face right, -1 to face left. A def with left-facing art (animsLeft) switches to those clips; one
    // without flips the sheet when that's opposite the art's facing.
    face(dir) {
        if (dir == _side && _faced == true) return
        _faced = true
        _side = dir
        var flip = FlipBit.NONE
        if (_def.animsLeft == null && dir != _def.facing) flip = FlipBit.HORIZONTAL
        Sprite.setFlipped(_entity, flip)
        if (_weapon != null) Sprite.setFlipped(_weapon, flip)
        _clip = null
    }

    anims_ { (_side == -1 && _def.animsLeft != null) ? _def.animsLeft : _def.anims }

    // The key of the clip for `role` at `duration` seconds (null = the sheet's own length), registering it on first
    // use. A role the sheet lacks falls back to its generic role, then to idle.
    clip_(role, duration, loop) {
        var anims = anims_
        if (!anims.containsKey(role)) {
            var generic = SheetAnims.fallbackRole(role)
            role = (generic != null && anims.containsKey(generic)) ? generic : "idle"
        }
        var frames = anims[role]
        var seconds = duration == null ? frames[2] : duration
        var side = (_side == -1 && _def.animsLeft != null) ? "left" : "right"
        var key = "%(role)|%(seconds)|%(loop)|%(side)"
        if (!_registered.containsKey(key)) {
            _anim.addAnimation(key, SpriteAnimationDefinition.new(frames[0], frames[1], seconds, loop))
            _registered[key] = true
        }
        return key
    }

    // Plays a looping / free-running clip (only restarting it when it changes).
    play_(role, duration, loop) {
        var key = clip_(role, duration, loop)
        if (key != _clip) {
            _clip = key
            SpriteAnimation.forcePlayAnimation(_entity, key)
        }
    }

    // The arena entrance: a run on the spot while BattleStage slides everyone onto their marks.
    playEntrance() { play_("entrance", null, true) }
    playIdle() { play_("idle", null, true) }

    // --- every frame ---------------------------------------------------------------------

    // dt: game seconds since the last frame. Reads the Combatant and shows it.
    update(dt) {
        var combatant = _combatant
        var now = Time.scaledTime
        if (_defeatedAt != null) {
            var t = Math.min((now - _defeatedAt) / DEFEAT_TIME, 1)
            setColor_(Vec4.new(HURT_TINT.x, HURT_TINT.y, HURT_TINT.z, 255 * (1 - t)))
            if (t >= 1) _gone = true
            syncWeapon_()
            return
        }

        face(combatant.facing)

        // Position: slide toward the cell, lean into an attack around its impact, hop on a dodge.
        var gap = Vec3.distance(_pos, _target)
        if (gap > 0.01) {
            var step = SLIDE_SPEED * dt
            _pos = gap <= step ? _target : Vec3.lerp(_pos, _target, step / gap)
        }
        var lean = 0
        var move = combatant.move
        if (move != null && move.isAttack && move.impact != null && combatant.stun <= 0) {
            var pastImpact = combatant.progress - move.impact
            if (pastImpact < 0) {
                lean = -2 * Math.clamp01(1 + pastImpact / 0.25)             // a small draw-back just before the hit
                if (pastImpact > -0.08) lean = LEAN * (1 + pastImpact / 0.08)
            } else {
                lean = LEAN * Math.clamp01(1 - pastImpact / 0.25)
            }
        }
        var hop = now < _hopUntil ? -6 * Math.sin((1 - (_hopUntil - now) / 0.3) * Math.pi) : 0
        WorldTransform.setPosition(_entity, Vec3.new(_pos.x + lean * combatant.facing, _pos.y + hop, 0))

        // Animation.
        if (combatant.stun > 0) {
            if (!_wasStunned) {
                _clip = clip_("hurt", 0.35, false)
                SpriteAnimation.forcePlayAnimation(_entity, _clip)
            }
            _wasStunned = true
        } else {
            _wasStunned = false
            if (move == null || move.isHold) {
                play_(gap > 0.5 ? "step" : "idle", null, true)
            } else {
                var key = clip_(move.anim, move.completion, false)
                _clip = key
                SpriteAnimation.playAnimationAt(_entity, key, Math.clamp01(combatant.progress / move.completion))
            }
        }

        // Tint: what the combatant is doing right now.
        var color = _base
        if (combatant.stun > 0) {
            color = HURT_TINT
        } else if (combatant.guardNow > 0) {
            color = Battler.mix_(_base, GUARD_TINT, 0.35 + 0.65 * combatant.guardNow)
        } else if (combatant.exposedNow) {
            color = Battler.mix_(_base, EXPOSED_TINT, 0.6 + 0.4 * Math.sin(now * 14))
        } else if (combatant.burn > 0) {
            color = Battler.mix_(_base, BURN_TINT, 0.5 + 0.3 * Math.sin(now * 9))
        } else if (combatant.chill > 0 || combatant.frozen > 0) {
            color = Battler.mix_(_base, CHILL_TINT, combatant.frozen > 0 ? 1 : 0.6)
        }
        // Dodging ghosts the battler; Soul Step's dodge turns it into flickering soul light, pouring off sparks.
        var soulStepping = combatant.dodgingNow && move != null && move.name == SOUL_STEP
        if (soulStepping) {
            var tint = SoulFx.tint
            color = Vec4.new(tint.x, tint.y, tint.z, 125 + (35 * (now * 30).sin).round)
        } else if (combatant.dodgingNow) {
            color = Vec4.new(color.x, color.y, color.z, 120)
        }
        showSoulStep_(soulStepping)
        setColor_(color)
        syncWeapon_()
    }

    // The soul sparks of a Soul Step: their emitter is made the first time one is needed, and only told when they
    // start or stop.
    showSoulStep_(on) {
        if (on == (_soulLit == true)) return
        _soulLit = on
        if (_soulLight == null) _soulLight = SoulFx.lightEmitter(_entity, SOUL_SPARK_HEIGHT)
        SoulFx.setLit(_soulLight, on ? SOUL_SPARK_RATE : 0)
    }

    static mix_(from, to, t) {
        var k = Math.clamp01(t)
        return Vec4.new(from.x + (to.x - from.x) * k, from.y + (to.y - from.y) * k, from.z + (to.z - from.z) * k, 255)
    }

    setColor_(color) {
        var sprite = Sprite.get(_entity)
        if (sprite != null) sprite.color = color
        if (_weapon != null) {
            var weaponSprite = Sprite.get(_weapon)
            if (weaponSprite != null) weaponSprite.color = Vec4.new(255, 255, 255, color.w)
        }
    }

    // The weapon layer shows the same frame as the body.
    syncWeapon_() {
        if (_weapon == null) return
        var body = Sprite.get(_entity)
        var weaponSprite = Sprite.get(_weapon)
        if (body != null && weaponSprite != null) weaponSprite.index = body.index
    }

    // --- reactions -------------------------------------------------------------------------

    // A quick hop: the dodge that let a hit pass.
    hop() { _hopUntil = Time.scaledTime + 0.3 }

    // Falls and fades out; `gone` turns true once it has.
    defeat() {
        if (_defeatedAt != null) return
        _defeatedAt = Time.scaledTime
        _clip = clip_("defeat", DEFEAT_TIME, false)
        SpriteAnimation.forcePlayAnimation(_entity, _clip)
    }

    // Fires one of Combat/Config/MoveParticles.wren's presets on this battler.
    playImpact(particleId) {
        MoveParticles.trigger(_emitter, particleId)
    }

    // Floating text over the battler - a damage number, a heal, "Blocked" - in `color` (a Vec4), rising and fading.
    popText(text, color) {
        _textEmitter.startColor = color
        _textEmitter.endColor = Vec4.new(color.x, color.y, color.z, 0)
        _textEmitter.spawnText(text)
    }

    // The dust cloud a hit throws up: tinted by `element` (null = plain dust), smaller for a blocked or weak hit.
    playHitCloud(element, blocked, amount) {
        var strength = amount / CLOUD_FULL_DAMAGE
        if (blocked) strength = strength * 0.5
        MoveParticles.hitCloud(_cloudEmitter, element, strength)
    }

    teardown() {
        if (_textFont != null) {
            _textFont.unload()
            _textFont = null
        }
        if (GameObject.isValid(_entity)) {
            GameObject.destroy(_entity)
        }
    }
}
