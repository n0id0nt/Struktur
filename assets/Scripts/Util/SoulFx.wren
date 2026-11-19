// Util/SoulFx.wren
// The look of the soul - the fragment, Soul Sight and Soul Step share it, so they read as the same thing:
//   lightEmitter / setLit - a child particle emitter of rising blue sparks (FX/Particle/Spark.png, played over each
//       spark's life). The player gives it off while Soul Sight is on (States/GameWorldState.wren), a battler while
//       Soul Step's untouchable window is open (Combat/Battler.wren), the fragment as it drifts (GameObjects/
//       SoulFragment.wren).
//   burst - a handful of the same sparks at once (the fragment sinking in).
//   tint - the soul's colour, for whatever glows with it (a hidden door or plate seen with Soul Sight, a Soul Step).
// (Not the jam's Shaders/SoulEffect: it works in whole-texture uvs, so on a cell of a sprite sheet it samples the
// neighbouring cells and turns to rainbow noise.)
import "gameObject" for GameObject
import "gameObjectComponents" for LocalTransform, ParticleEmitter, ParticleAnimationMode, RenderLayer
import "animation" for SpriteAnimationDefinition
import "resourceManager" for Texture
import "math" for Vec2, Vec3, Vec4

var SPARK_SHEET = "FX/Particle/Spark.png"   // 7 cells of 10 x 8, one row: a pinprick swelling to a glow and fading
var SPARK_COLUMNS = 7
var START_COLOR = Vec4.new(150, 230, 255, 235)
var END_COLOR = Vec4.new(110, 190, 255, 0)
// The soul's colour for tints (a battler's Soul Step, a hidden thing seen with Soul Sight).
var SOUL_TINT = Vec4.new(130, 215, 255, 255)

class SoulFx {
    static tint { SOUL_TINT }

    // A child of `parent`, `height` px above its origin, giving off nothing until setLit. Returns the child entity
    // (component handles go stale - look the emitter up through it).
    static lightEmitter(parent, height) {
        var entity = GameObject.create("SoulLight", parent)
        LocalTransform.setPosition(entity, Vec3.new(0, -height, 0))
        var texture = Texture.load(SPARK_SHEET)
        var emitter = ParticleEmitter.create(entity, texture)
        texture.unload()
        emitter.columns = SPARK_COLUMNS
        emitter.rows = 1
        emitter.looping = true
        emitter.emissionRate = 0
        emitter.burstCount = 0
        emitter.maxParticles = 40
        emitter.spawnRadius = 7
        emitter.lifetimeMin = 0.7
        emitter.lifetimeMax = 1.1
        emitter.velocityMin = Vec2.new(-8, -28)
        emitter.velocityMax = Vec2.new(8, -12)
        emitter.acceleration = Vec2.new(0, 0)
        emitter.startScale = 1
        emitter.endScale = 0.7
        emitter.rotationSpeedMin = 0
        emitter.rotationSpeedMax = 0
        emitter.startColor = START_COLOR
        emitter.endColor = END_COLOR
        emitter.additive = true
        emitter.layer = RenderLayer.FOREGROUND
        emitter.setAnimation(SpriteAnimationDefinition.new(0, SPARK_COLUMNS, 1.0, false), ParticleAnimationMode.LIFETIME)
        return entity
    }

    // Sparks per second from a lightEmitter (0 stops it; sparks already out fade on their own).
    static setLit(entity, rate) {
        if (entity == null || !GameObject.isValid(entity)) return
        var emitter = ParticleEmitter.get(entity)
        if (emitter != null) emitter.emissionRate = rate
    }

    // `count` sparks at once (queued - they appear on the next particle update, alongside any still alive).
    static burst(entity, count) {
        if (entity == null || !GameObject.isValid(entity)) return
        var emitter = ParticleEmitter.get(entity)
        if (emitter != null) emitter.spawn(count)
    }
}
