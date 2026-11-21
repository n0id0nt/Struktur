// GameObjects/ParticleEffect.wren
// An LDtk "ParticleEffect" entity: a particle emitter set up entirely from its fields. The game plays it from start(),
// the editor's level viewer from preview() - both build it with createEmitter, so the viewer shows what the game will.
// A field that's missing or left empty keeps the emitter's default (Component::ParticleEmitter).
//   Texture (FilePath, png)                           - the particle sheet (default FX/Particle/Spark.png)
//   Columns, Rows (Int)                               - the sheet's grid of cells
//   AnimationMode (ParticleAnimationMode)             - None / Once / Loop / Lifetime (see ParticleAnimationMode)
//   AnimationStart, AnimationEnd (Int), AnimationTime (Float) - the cells each particle plays (end excluded), seconds
//   Looping (Bool), EmissionRate (Float)              - keep emitting this many particles a second
//   BurstCount (Int)                                  - particles spawned at once when it starts
//   MaxParticles (Int), SpawnRadius (Float)           - pool size; 0 = all from one point, else within this radius
//   VelocityMinX/Y, VelocityMaxX/Y (Float)            - each particle's start velocity is picked between these (px/s)
//   AccelerationX/Y (Float)                           - e.g. gravity (px/s^2)
//   LifetimeMin, LifetimeMax (Float)                  - seconds
//   StartColor, EndColor (Color), StartAlpha, EndAlpha (Int 0-255) - colour over each particle's life
//   StartScale, EndScale (Float)                      - size over each particle's life
//   RotationSpeedMin, RotationSpeedMax (Float)        - degrees a second
//   Additive (Bool)                                   - glow-style additive blending
//   Layer (ParticleRenderLayer), OrderInLayer (Float) - draw order
import "gameObjectComponents" for ParticleEmitter, ParticleAnimationMode, RenderLayer
import "animation" for SpriteAnimationDefinition
import "resourceManager" for Texture
import "math" for Vec2, Vec4
import "Util/Colors" for Colors

var DEFAULT_TEXTURE = "FX/Particle/Spark.png"
// Component::ParticleEmitter's defaults for the vector/colour settings - written out because the emitter's zero-arg
// Vec2/Vec4 getters can't be read back from Wren yet (see the wrenEnsureSlots note in the bindings).
var DEFAULT_VELOCITY_MIN = Vec2.new(-50, -50)
var DEFAULT_VELOCITY_MAX = Vec2.new(50, 50)
var DEFAULT_ACCELERATION = Vec2.new(0, 0)
var DEFAULT_START_COLOR = Vec4.new(255, 255, 255, 255)
var DEFAULT_END_COLOR = Vec4.new(255, 255, 255, 0)
var ANIMATION_MODES = {
    "None": ParticleAnimationMode.NONE,
    "Once": ParticleAnimationMode.ONCE,
    "Loop": ParticleAnimationMode.LOOP,
    "Lifetime": ParticleAnimationMode.LIFETIME,
}
var RENDER_LAYERS = {
    "BackgroundFar": RenderLayer.BACKGROUND_FAR,
    "BackgroundMid": RenderLayer.BACKGROUND_MID,
    "Entities": RenderLayer.ENTITIES,
    "BackgroundOverlay": RenderLayer.BACKGROUND_OVERLAY,
    "Foreground": RenderLayer.FOREGROUND,
}

class ParticleEffect {
    construct new(entity, args) {
        _entity = entity
        _args = args
    }

    start() {
        ParticleEffect.createEmitter(_entity, _args)
    }

    // Level viewer (editor): the same emitter - the viewer runs the particle simulation, so it plays as in the game.
    static preview(entity, args) {
        ParticleEffect.createEmitter(entity, args)
    }

    static createEmitter(entity, args) {
        var texture = Texture.load(ParticleEffect.text_(args, "Texture", DEFAULT_TEXTURE))
        if (texture == null || !texture.isValid()) {
            texture = Texture.load(DEFAULT_TEXTURE)
        }
        var emitter = ParticleEmitter.create(entity, texture)
        texture.unload()

        emitter.columns = ParticleEffect.number_(args, "Columns", emitter.columns)
        emitter.rows = ParticleEffect.number_(args, "Rows", emitter.rows)
        emitter.maxParticles = ParticleEffect.number_(args, "MaxParticles", emitter.maxParticles)
        emitter.looping = ParticleEffect.boolean_(args, "Looping", emitter.looping)
        emitter.emissionRate = ParticleEffect.number_(args, "EmissionRate", emitter.emissionRate)
        emitter.burstCount = ParticleEffect.number_(args, "BurstCount", emitter.burstCount)
        emitter.spawnRadius = ParticleEffect.number_(args, "SpawnRadius", emitter.spawnRadius)

        emitter.velocityMin = Vec2.new(ParticleEffect.number_(args, "VelocityMinX", DEFAULT_VELOCITY_MIN.x),
                                       ParticleEffect.number_(args, "VelocityMinY", DEFAULT_VELOCITY_MIN.y))
        emitter.velocityMax = Vec2.new(ParticleEffect.number_(args, "VelocityMaxX", DEFAULT_VELOCITY_MAX.x),
                                       ParticleEffect.number_(args, "VelocityMaxY", DEFAULT_VELOCITY_MAX.y))
        emitter.acceleration = Vec2.new(ParticleEffect.number_(args, "AccelerationX", DEFAULT_ACCELERATION.x),
                                        ParticleEffect.number_(args, "AccelerationY", DEFAULT_ACCELERATION.y))

        emitter.lifetimeMin = ParticleEffect.number_(args, "LifetimeMin", emitter.lifetimeMin)
        emitter.lifetimeMax = ParticleEffect.number_(args, "LifetimeMax", emitter.lifetimeMax)
        emitter.startColor = ParticleEffect.color_(args, "StartColor", "StartAlpha", DEFAULT_START_COLOR)
        emitter.endColor = ParticleEffect.color_(args, "EndColor", "EndAlpha", DEFAULT_END_COLOR)
        emitter.startScale = ParticleEffect.number_(args, "StartScale", emitter.startScale)
        emitter.endScale = ParticleEffect.number_(args, "EndScale", emitter.endScale)

        // Authored in degrees a second; the emitter spins in radians.
        var degreesToRadians = Num.pi / 180
        emitter.rotationSpeedMin = ParticleEffect.number_(args, "RotationSpeedMin", 0) * degreesToRadians
        emitter.rotationSpeedMax = ParticleEffect.number_(args, "RotationSpeedMax", 0) * degreesToRadians

        emitter.additive = ParticleEffect.boolean_(args, "Additive", emitter.additive)
        if (RENDER_LAYERS.containsKey(args["Layer"])) {
            emitter.layer = RENDER_LAYERS[args["Layer"]]
        }
        emitter.orderInLayer = ParticleEffect.number_(args, "OrderInLayer", emitter.orderInLayer)

        var mode = ANIMATION_MODES.containsKey(args["AnimationMode"]) ? ANIMATION_MODES[args["AnimationMode"]] : ParticleAnimationMode.NONE
        if (mode != ParticleAnimationMode.NONE) {
            var animation = SpriteAnimationDefinition.new(ParticleEffect.number_(args, "AnimationStart", 0),
                                                          ParticleEffect.number_(args, "AnimationEnd", 1),
                                                          ParticleEffect.number_(args, "AnimationTime", 1),
                                                          mode == ParticleAnimationMode.LOOP)
            emitter.setAnimation(animation, mode)
        }
        return emitter
    }

    // The field's value if it's set (and the right type), else the fallback.
    static number_(args, name, fallback) { args[name] is Num ? args[name] : fallback }
    static boolean_(args, name, fallback) { args[name] is Bool ? args[name] : fallback }
    static text_(args, name, fallback) {
        var value = args[name]
        return value is String && value != "" ? value : fallback
    }

    // A Color field plus its 0-255 alpha field, or the fallback when the colour isn't set.
    static color_(args, colorName, alphaName, fallback) {
        var alpha = ParticleEffect.number_(args, alphaName, fallback.w)
        var color = Colors.fromHex(args[colorName], alpha)
        return color != null ? color : Vec4.new(fallback.x, fallback.y, fallback.z, alpha)
    }

    update() {}

    onDestroy() {}

    onEvent(type, data) {}

    getEntity() { _entity }
}
