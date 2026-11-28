// GameObjects/ArrowShot.wren
// An arrow loosed in the overworld (Kira at the training grounds): a single arrow particle (FX/Projectile/Arrow.png,
// pointing east) that flies from where the bow is to where it lands, then a puff of dust there. Spawned by the
// "shootArrow" dialogue command (Dialogue/RegisteredFunctions/Commands.wren), timed to the shooter's Shoot clip.
// args: "FromX" / "FromY" - where it leaves the bow (px); "ToX" / "ToY" - where it hits; "Delay" - seconds before it's
// loosed (the draw); "Speed" - px a second.
import "gameObject" for GameObject
import "gameObjectComponents" for ParticleEmitter, ParticleAnimationMode, WorldTransform, LocalTransform, RenderLayer
import "animation" for SpriteAnimationDefinition
import "resourceManager" for Texture
import "math" for Vec2, Vec3, Vec4
import "app" for Time

var ARROW_TEXTURE = "FX/Projectile/Arrow.png"
var DUST_SHEET = "FX/Particle/Spark.png"
var DUST_COLUMNS = 7
var DUST_COUNT = 6
var ARROW_COLOR = Vec4.new(255, 255, 255, 255)
var DUST_START = Vec4.new(235, 220, 190, 230)
var DUST_END = Vec4.new(200, 180, 150, 0)
var DUST_SECONDS = 0.5
var DRAW_ORDER = 10000     // over the y-sorted characters and the target it hits (particles aren't y-sorted)

class ArrowShot {
    construct new(entity, args) {
        _entity = entity
        _from = Vec2.new(args["FromX"], args["FromY"])
        _to = Vec2.new(args["ToX"], args["ToY"])
        _delay = args["Delay"] is Num ? args["Delay"] : 0
        var speed = args["Speed"] is Num ? args["Speed"] : 240
        _flight = (Vec2.distance(_from, _to) / speed).max(0.05)
        _startedAt = null
        _loosedAt = null
        _landed = false
    }

    start() {
        WorldTransform.setPosition(_entity, Vec3.new(_from.x, _from.y, 0))
        var texture = Texture.load(ARROW_TEXTURE)
        var arrow = ParticleEmitter.create(_entity, texture)
        texture.unload()
        arrow.columns = 1
        arrow.rows = 1
        arrow.looping = false
        arrow.emissionRate = 0
        arrow.burstCount = 0
        arrow.maxParticles = 1
        arrow.spawnRadius = 0
        var velocity = Vec2.new((_to.x - _from.x) / _flight, (_to.y - _from.y) / _flight)
        arrow.velocityMin = velocity
        arrow.velocityMax = velocity
        arrow.acceleration = Vec2.new(0, 0)
        arrow.lifetimeMin = _flight
        arrow.lifetimeMax = _flight
        arrow.startColor = ARROW_COLOR
        arrow.endColor = ARROW_COLOR
        arrow.startScale = 1
        arrow.endScale = 1
        arrow.rotationSpeedMin = 0
        arrow.rotationSpeedMax = 0
        arrow.layer = RenderLayer.ENTITIES
        arrow.orderInLayer = DRAW_ORDER

        // The dust where it lands - a child at the hit point, puffed once the arrow gets there.
        _dust = GameObject.create("ArrowDust", _entity)
        LocalTransform.setPosition(_dust, Vec3.new(_to.x - _from.x, _to.y - _from.y, 0))
        var dustTexture = Texture.load(DUST_SHEET)
        var dust = ParticleEmitter.create(_dust, dustTexture)
        dustTexture.unload()
        dust.columns = DUST_COLUMNS
        dust.rows = 1
        dust.looping = false
        dust.emissionRate = 0
        dust.burstCount = 0
        dust.maxParticles = DUST_COUNT
        dust.spawnRadius = 2
        dust.velocityMin = Vec2.new(-30, -26)
        dust.velocityMax = Vec2.new(-6, 10)
        dust.acceleration = Vec2.new(0, 40)
        dust.lifetimeMin = DUST_SECONDS * 0.6
        dust.lifetimeMax = DUST_SECONDS
        dust.startColor = DUST_START
        dust.endColor = DUST_END
        dust.startScale = 1
        dust.endScale = 0.5
        dust.rotationSpeedMin = 0
        dust.rotationSpeedMax = 0
        dust.layer = RenderLayer.ENTITIES
        dust.orderInLayer = DRAW_ORDER
        dust.setAnimation(SpriteAnimationDefinition.new(0, DUST_COLUMNS, 1.0, false), ParticleAnimationMode.LIFETIME)
        _startedAt = Time.scaledTime
    }

    update() {
        var now = Time.scaledTime
        if (_loosedAt == null) {
            if (now - _startedAt < _delay) return
            _loosedAt = now
            ParticleEmitter.get(_entity).spawn(1)
            return
        }
        if (!_landed && now - _loosedAt >= _flight) {
            _landed = true
            ParticleEmitter.get(_dust).spawn(DUST_COUNT)
        }
        if (now - _loosedAt >= _flight + DUST_SECONDS + 0.1) GameObject.destroy(_entity)
    }

    onDestroy() {}

    onEvent(type, data) {}

    getEntity() { _entity }
}
